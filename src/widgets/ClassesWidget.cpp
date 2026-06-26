#include "ClassesWidget.h"

#include "common/Helpers.h"
#include "common/SvgIconEngine.h"
#include "core/MainWindow.h"
#include "dialogs/EditMethodDialog.h"
#include "ui_ListDockWidget.h"

#include <QComboBox>
#include <QInputDialog>
#include <QList>
#include <QMenu>
#include <QMouseEvent>
#include <QShortcut>

QVariant ClassesModel::headerData(int section, Qt::Orientation, int role) const
{
    switch (role) {
    case Qt::DisplayRole:
        switch (section) {
        case NAME:
            return tr("Name");
        case REAL_NAME:
            return tr("Real Name");
        case TYPE:
            return tr("Type");
        case OFFSET:
            return tr("Offset");
        case VTABLE:
            return tr("VTable");
        default:
            return QVariant();
        }
    default:
        return QVariant();
    }
}

RVA ClassesModel::address(const QModelIndex &index) const
{
    const QVariant v = data(index, offsetRole);
    return v.isValid() ? v.toULongLong() : RVA_INVALID;
}

QString ClassesModel::name(const QModelIndex &index) const
{
    return data(index, nameRole).toString();
}

BinClassesModel::BinClassesModel(QObject *parent) : ClassesModel(parent) {}

void BinClassesModel::setClasses(const QList<BinClassDescription> &classes)
{
    beginResetModel();
    this->classes = classes;
    endResetModel();
}

QModelIndex BinClassesModel::index(int row, int column, const QModelIndex &parent) const
{
    if (!parent.isValid()) {
        return createIndex(row, column, (quintptr)0); // root function nodes have id = 0
    }

    return createIndex(row, column,
                       (quintptr)parent.row() + 1); // sub-nodes have id = class index + 1
}

QModelIndex BinClassesModel::parent(const QModelIndex &index) const
{
    if (!index.isValid()) {
        return {};
    }

    if (index.internalId() == 0) { // root function node
        return {};
    } else { // sub-node
        return this->index((int)(index.internalId() - 1), 0);
    }
}

int BinClassesModel::rowCount(const QModelIndex &parent) const
{
    if (!parent.isValid()) { // root
        return classes.count();
    }

    if (parent.internalId() == 0) { // methods/fields
        const BinClassDescription *cls = &classes.at(parent.row());
        return cls->baseClasses.length() + cls->methods.length() + cls->fields.length();
    }

    return 0; // below methods/fields
}

int BinClassesModel::columnCount(const QModelIndex &) const
{
    return Columns::COUNT;
}

QVariant BinClassesModel::data(const QModelIndex &index, int role) const
{
    const BinClassDescription *cls;
    const BinClassMethodDescription *meth = nullptr;
    const BinClassFieldDescription *field = nullptr;
    const BinClassBaseClassDescription *base = nullptr;
    if (index.internalId() == 0) { // class row
        if (index.row() >= classes.count()) {
            return QVariant();
        }

        cls = &classes.at(index.row());
    } else { // method/field/base row
        cls = &classes.at(static_cast<int>(index.internalId() - 1));

        if (index.row()
            >= cls->baseClasses.length() + cls->methods.length() + cls->fields.length()) {
            return QVariant();
        }

        if (index.row() < cls->baseClasses.length()) {
            base = &cls->baseClasses[index.row()];
        } else if (index.row() - cls->baseClasses.length() < cls->methods.length()) {
            meth = &cls->methods[index.row() - cls->baseClasses.length()];
        } else {
            field = &cls->fields[index.row() - cls->baseClasses.length() - cls->methods.length()];
        }
    }

    if (meth) {
        switch (role) {
        case Qt::DisplayRole:
            switch (index.column()) {
            case NAME:
                return meth->name;
            case TYPE:
                return tr("method");
            case OFFSET:
                return meth->addr == RVA_INVALID ? QString() : rzAddressString(meth->addr);
            case VTABLE:
                return meth->vtableOffset < 0 ? QString() : QString("+%1").arg(meth->vtableOffset);
            default:
                return QVariant();
            }
        case offsetRole:
            return QVariant::fromValue(meth->addr);
        case nameRole:
            return meth->name;
        case typeRole:
            return QVariant::fromValue(RowType::Method);
        default:
            return QVariant();
        }
    } else if (field) {
        switch (role) {
        case Qt::DisplayRole:
            switch (index.column()) {
            case NAME:
                return field->name;
            case TYPE:
                return tr("field");
            case OFFSET:
                return field->addr == RVA_INVALID ? QString() : rzAddressString(field->addr);
            default:
                return QVariant();
            }
        case offsetRole:
            return QVariant::fromValue(field->addr);
        case nameRole:
            return field->name;
        case typeRole:
            return QVariant::fromValue(RowType::Field);
        default:
            return QVariant();
        }
    } else if (base) {
        switch (role) {
        case Qt::DisplayRole:
            switch (index.column()) {
            case NAME:
                return base->name;
            case TYPE:
                return tr("base class");
            case OFFSET:
                return QString("+%1").arg(base->offset);
            default:
                return QVariant();
            }
        case nameRole:
            return base->name;
        case typeRole:
            return QVariant::fromValue(RowType::Base);
        default:
            return QVariant();
        }
    } else {
        switch (role) {
        case Qt::DisplayRole:
            switch (index.column()) {
            case NAME:
                return cls->name;
            case TYPE:
                return tr("class");
            case OFFSET:
                return cls->addr == RVA_INVALID ? QString() : rzAddressString(cls->addr);
            case VTABLE:
                return cls->vtableAddr == RVA_INVALID ? QString()
                                                      : rzAddressString(cls->vtableAddr);
            default:
                return QVariant();
            }
        case offsetRole:
            return QVariant::fromValue(cls->addr);
        case nameRole:
            return cls->name;
        case typeRole:
            return QVariant::fromValue(RowType::Class);
        default:
            return QVariant();
        }
    }
}

AnalysisClassesModel::AnalysisClassesModel(CutterDockWidget *parent)
    : ClassesModel(parent),
      refreshDeferrer(parent->createRefreshDeferrer([this]() { this->refreshAll(); })),
      attrs(new QMap<QString, QVector<Attribute>>)
{
    // Just use a simple refresh deferrer. If an event was triggered in the background, simply
    // refresh everything later.

    connect(Core(), &CutterCore::refreshAll, this, &AnalysisClassesModel::refreshAll);
    connect(Core(), &CutterCore::codeRebased, this, &AnalysisClassesModel::refreshAll);
    connect(Core(), &CutterCore::classNew, this, &AnalysisClassesModel::classNew);
    connect(Core(), &CutterCore::classDeleted, this, &AnalysisClassesModel::classDeleted);
    connect(Core(), &CutterCore::classRenamed, this, &AnalysisClassesModel::classRenamed);
    connect(Core(), &CutterCore::classAttrsChanged, this, &AnalysisClassesModel::classAttrsChanged);

    refreshAll();
}

void AnalysisClassesModel::refreshAll()
{
    if (!refreshDeferrer->attemptRefresh(nullptr)) {
        return;
    }

    beginResetModel();
    attrs->clear();
    classes = Core()->getAllAnalysisClasses(true); // must be sorted
    endResetModel();
}

void AnalysisClassesModel::classNew(const QString &cls)
{
    if (!refreshDeferrer->attemptRefresh(nullptr)) {
        return;
    }

    // find the destination position using binary search and add the row
    auto it = std::lower_bound(classes.begin(), classes.end(), cls);
    const int index = it - classes.begin();
    beginInsertRows(QModelIndex(), index, index);
    classes.insert(it, cls);
    endInsertRows();
}

void AnalysisClassesModel::classDeleted(const QString &cls)
{
    if (!refreshDeferrer->attemptRefresh(nullptr)) {
        return;
    }

    // find the position using binary search and remove the row
    auto it = std::lower_bound(classes.begin(), classes.end(), cls);
    if (it == classes.end() || *it != cls) {
        return;
    }
    const int index = it - classes.begin();
    beginRemoveRows(QModelIndex(), index, index);
    classes.erase(it);
    endRemoveRows();
}

void AnalysisClassesModel::classRenamed(const QString &oldName, const QString &newName)
{
    if (!refreshDeferrer->attemptRefresh(nullptr)) {
        return;
    }

    auto oldIt = std::lower_bound(classes.begin(), classes.end(), oldName);
    if (oldIt == classes.end() || *oldIt != oldName) {
        return;
    }
    auto newIt = std::lower_bound(classes.begin(), classes.end(), newName);
    const int oldRow = oldIt - classes.begin();
    int newRow = newIt - classes.begin();
    // oldRow == newRow means the name stayed the same.
    // oldRow == newRow - 1 means the name changed, but the row stays the same.
    if (oldRow != newRow && oldRow != newRow - 1) {
        beginMoveRows(QModelIndex(), oldRow, oldRow, QModelIndex(), newRow);
        classes.erase(oldIt);
        // iterators are invalid now, so we calculate the new position from the rows.
        if (oldRow < newRow) {
            // if we move down, we need to account for the removed old element above.
            newRow--;
        }
        classes.insert(newRow, newName);
        endMoveRows();
    } else if (oldRow == newRow - 1) { // class name changed, but not the row
        newRow--;
        classes[newRow] = newName;
    }
    emit dataChanged(index(newRow, 0), index(newRow, 0));
}

void AnalysisClassesModel::classAttrsChanged(const QString &cls)
{
    if (!refreshDeferrer->attemptRefresh(nullptr)) {
        return;
    }

    auto it = std::lower_bound(classes.begin(), classes.end(), cls);
    if (it == classes.end() || *it != cls) {
        return;
    }
    const QPersistentModelIndex persistentIndex =
            QPersistentModelIndex(index(it - classes.begin(), 0));
    layoutAboutToBeChanged({ persistentIndex });
    attrs->remove(cls);
    layoutChanged({ persistentIndex });
}

const QVector<AnalysisClassesModel::Attribute> &
AnalysisClassesModel::getAttrs(const QString &cls) const
{
    auto it = attrs->find(cls);
    if (it != attrs->end()) {
        return it.value();
    }

    QVector<AnalysisClassesModel::Attribute> clsAttrs;
    const QList<AnalysisBaseClassDescription> bases = Core()->getAnalysisClassBaseClasses(cls);
    const QList<AnalysisMethodDescription> meths = Core()->getAnalysisClassMethods(cls);
    const QList<AnalysisVTableDescription> vtables = Core()->getAnalysisClassVTables(cls);
    clsAttrs.reserve(bases.size() + meths.size() + vtables.size());

    for (const AnalysisBaseClassDescription &base : bases) {
        clsAttrs.push_back(Attribute(Attribute::Type::Base, QVariant::fromValue(base)));
    }

    for (const AnalysisVTableDescription &vtable : vtables) {
        clsAttrs.push_back(Attribute(Attribute::Type::VTable, QVariant::fromValue(vtable)));
    }

    for (const AnalysisMethodDescription &meth : meths) {
        clsAttrs.push_back(Attribute(Attribute::Type::Method, QVariant::fromValue(meth)));
    }

    return attrs->insert(cls, clsAttrs).value();
}

QModelIndex AnalysisClassesModel::index(int row, int column, const QModelIndex &parent) const
{
    if (!parent.isValid()) {
        return createIndex(row, column, (quintptr)0); // root function nodes have id = 0
    }

    return createIndex(row, column,
                       (quintptr)parent.row() + 1); // sub-nodes have id = class index + 1
}

QModelIndex AnalysisClassesModel::parent(const QModelIndex &index) const
{
    if (!index.isValid()) {
        return {};
    }

    if (index.internalId() == 0) { // root function node
        return {};
    } else { // sub-node
        return this->index((int)(index.internalId() - 1), 0);
    }
}

int AnalysisClassesModel::rowCount(const QModelIndex &parent) const
{
    if (!parent.isValid()) { // root
        return classes.count();
    }

    if (parent.internalId() == 0) { // methods/fields
        return getAttrs(classes[parent.row()]).size();
    }

    return 0; // below methods/fields
}

bool AnalysisClassesModel::hasChildren(const QModelIndex &parent) const
{
    return !parent.isValid() || !parent.parent().isValid();
}

int AnalysisClassesModel::columnCount(const QModelIndex &) const
{
    return Columns::COUNT;
}

QVariant AnalysisClassesModel::data(const QModelIndex &index, int role) const
{
    if (index.internalId() == 0) { // class row
        if (index.row() >= classes.count()) {
            return QVariant();
        }

        const QString cls = classes.at(index.row());
        switch (role) {
        case Qt::DisplayRole:
            switch (index.column()) {
            case NAME:
                return cls;
            case TYPE:
                return tr("class");
            default:
                return QVariant();
            }
        case typeRole:
            return QVariant::fromValue(RowType::Class);
        case nameRole:
            return cls;
        default:
            return QVariant();
        }
    } else { // method/field/base row
        const QString cls = classes.at(static_cast<int>(index.internalId() - 1));
        const Attribute &attr = getAttrs(cls)[index.row()];

        switch (attr.type) {
        case Attribute::Type::Base: {
            const auto base = attr.data.value<AnalysisBaseClassDescription>();
            switch (role) {
            case Qt::DisplayRole:
                switch (index.column()) {
                case NAME:
                    return base.className;
                case TYPE:
                    return tr("base");
                case OFFSET:
                    return QString("+%1").arg(base.offset);
                default:
                    return QVariant();
                }
            case Qt::DecorationRole:
                if (index.column() == NAME) {
                    return QIcon(new SvgIconEngine(QString(":/img/icons/home.svg"),
                                                   QPalette::WindowText));
                }
                return QVariant();
            case vTableRole:
                return -1;
            case nameRole:
                return base.className;
            case typeRole:
                return QVariant::fromValue(RowType::Base);
            default:
                return QVariant();
            }
            break;
        }
        case Attribute::Type::Method: {
            const auto meth = attr.data.value<AnalysisMethodDescription>();
            switch (role) {
            case Qt::DisplayRole:
                switch (index.column()) {
                case NAME:
                    return meth.name;
                case REAL_NAME:
                    return meth.realName;
                case TYPE:
                    return tr("method");
                case OFFSET:
                    return meth.addr == RVA_INVALID ? QString() : rzAddressString(meth.addr);
                case VTABLE:
                    return meth.vtableOffset < 0 ? QString()
                                                 : QString("+%1").arg(meth.vtableOffset);
                default:
                    return QVariant();
                }
            case Qt::DecorationRole:
                if (index.column() == NAME) {
                    return QIcon(new SvgIconEngine(QString(":/img/icons/fork.svg"),
                                                   QPalette::WindowText));
                }
                return QVariant();
            case vTableRole:
                return QVariant::fromValue(meth.vtableOffset);
            case offsetRole:
                return QVariant::fromValue(meth.addr);
            case nameRole:
                return meth.name;
            case realNameRole:
                return meth.realName;
            case typeRole:
                return QVariant::fromValue(RowType::Method);
            default:
                return QVariant();
            }
            break;
        }
        case Attribute::Type::VTable: {
            const auto vtable = attr.data.value<AnalysisVTableDescription>();
            switch (role) {
            case Qt::DisplayRole:
                switch (index.column()) {
                case NAME:
                    return "vtable";
                case TYPE:
                    return tr("vtable");
                case OFFSET:
                    return rzAddressString(vtable.addr);
                default:
                    return QVariant();
                }
            case Qt::DecorationRole:
                if (index.column() == NAME) {
                    return QIcon(new SvgIconEngine(QString(":/img/icons/list.svg"),
                                                   QPalette::WindowText));
                }
                return QVariant();
            case offsetRole:
                return QVariant::fromValue(vtable.addr);
            case typeRole:
                return QVariant::fromValue(RowType::VTable);
            default:
                return QVariant();
            }
            break;
        }
        }
    }
    return QVariant();
}

ClassesSortFilterProxyModel::ClassesSortFilterProxyModel(QObject *parent)
    : AddressableFilterProxyModel(nullptr, parent)
{
    setFilterCaseSensitivity(Qt::CaseInsensitive);
    setSortCaseSensitivity(Qt::CaseInsensitive);
}

bool ClassesSortFilterProxyModel::filterAcceptsRow(int row, const QModelIndex &parent) const
{
    if (parent.isValid()) {
        return true;
    }

    const QModelIndex index = sourceModel()->index(row, 0, parent);
    return qhelpers::filterStringContains(index.data(ClassesModel::nameRole).toString(), this);
}

bool ClassesSortFilterProxyModel::lessThan(const QModelIndex &left, const QModelIndex &right) const
{
    switch (left.column()) {
    case ClassesModel::OFFSET: {
        const RVA leftOffset = left.data(ClassesModel::offsetRole).toULongLong();
        const RVA rightOffset = right.data(ClassesModel::offsetRole).toULongLong();
        if (leftOffset != rightOffset) {
            return leftOffset < rightOffset;
        }
    }
    // fallthrough
    case ClassesModel::TYPE: {
        auto leftType = left.data(ClassesModel::typeRole).value<ClassesModel::RowType>();
        auto rightType = right.data(ClassesModel::typeRole).value<ClassesModel::RowType>();
        if (leftType != rightType) {
            return leftType < rightType;
        }
    }
    // fallthrough
    case ClassesModel::VTABLE: {
        auto leftVtable = left.data(ClassesModel::vTableRole).toLongLong();
        auto rightVtable = right.data(ClassesModel::vTableRole).toLongLong();
        if (leftVtable != rightVtable) {
            return leftVtable < rightVtable;
        }
    }
    // fallthrough
    case ClassesModel::NAME:
    default:
        const QString leftName = left.data(ClassesModel::nameRole).toString();
        const QString rightName = right.data(ClassesModel::nameRole).toString();
        return QString::compare(leftName, rightName, Qt::CaseInsensitive) < 0;
    }
}

bool ClassesSortFilterProxyModel::hasChildren(const QModelIndex &parent) const
{
    return !parent.isValid() || !parent.parent().isValid();
}

ClassesWidget::ClassesWidget(MainWindow *main)
    : ListDockWidget(main),
      proxyModel(new ClassesSortFilterProxyModel(this)),
      classSourceCombo(new QComboBox(this)),
      seekToVTableAction(tr("Seek to VTable"), this),
      editMethodAction(tr("Edit Method"), this),
      addMethodAction(tr("Add Method"), this),
      newClassAction(tr("Create new Class"), this),
      renameClassAction(tr("Rename Class"), this),
      deleteClassAction(tr("Delete Class"), this)
{
    setWindowTitle(tr("Classes"));
    setObjectName("ClassesWidget");

    ui->treeView->setIconSize(QSize(10, 10));

    setModels(proxyModel);

    // User an intermediate single-child layout to contain the combo box, otherwise
    // when the combo box is inserted directly, the entire vertical layout gets a
    // weird horizontal padding on macOS.
    auto *comboLayout = new QBoxLayout(QBoxLayout::Direction::LeftToRight, nullptr);
    comboLayout->addWidget(classSourceCombo);
    ui->verticalLayout->insertLayout(ui->verticalLayout->indexOf(ui->quickFilterView), comboLayout);
    classSourceCombo->addItem(tr("Binary Info (Fixed)"));
    classSourceCombo->addItem(tr("Analysis (Editable)"));
    classSourceCombo->setCurrentIndex(1);

    connect<void (QComboBox::*)(int)>(classSourceCombo, &QComboBox::currentIndexChanged, this,
                                      &ClassesWidget::refreshClasses);

    connect(&seekToVTableAction, &QAction::triggered, this,
            &ClassesWidget::seekToVTableActionTriggered);
    connect(&editMethodAction, &QAction::triggered, this,
            &ClassesWidget::editMethodActionTriggered);
    connect(&addMethodAction, &QAction::triggered, this, &ClassesWidget::addMethodActionTriggered);
    connect(&newClassAction, &QAction::triggered, this, &ClassesWidget::newClassActionTriggered);
    connect(&renameClassAction, &QAction::triggered, this,
            &ClassesWidget::renameClassActionTriggered);
    connect(&deleteClassAction, &QAction::triggered, this,
            &ClassesWidget::deleteClassActionTriggered);

    // Build context menu like this:
    //   class-related actions
    //   -- classesMethodsSeparator
    //   method-related actions
    //   -- separator
    //   default actions from AddressableItemList
    auto contextMenu = ui->treeView->getItemContextMenu();
    contextMenu->insertSeparator(contextMenu->actions().first());
    contextMenu->insertActions(contextMenu->actions().first(),
                               { &addMethodAction, &editMethodAction, &seekToVTableAction });
    classesMethodsSeparator = contextMenu->insertSeparator(contextMenu->actions().first());
    contextMenu->insertActions(classesMethodsSeparator,
                               { &newClassAction, &renameClassAction, &deleteClassAction });
    connect(contextMenu, &QMenu::aboutToShow, this, &ClassesWidget::updateActions);
    ui->treeView->setShowItemContextMenuWithoutAddress(true);

    refreshClasses();
}

ClassesWidget::~ClassesWidget() {}

ClassesWidget::Source ClassesWidget::getSource()
{
    switch (classSourceCombo->currentIndex()) {
    case 0:
        return Source::BIN;
    default:
        return Source::ANALYSIS;
    }
}

void ClassesWidget::refreshClasses()
{
    switch (getSource()) {
    case Source::BIN:
        if (!binModel) {
            proxyModel->setSourceModel(static_cast<AddressableItemModelI *>(nullptr));
            delete analysisModel;
            analysisModel = nullptr;
            binModel = new BinClassesModel(this);
            proxyModel->setSourceModel(static_cast<AddressableItemModelI *>(binModel));
        }
        binModel->setClasses(Core()->getAllClassesFromBin());
        break;
    case Source::ANALYSIS:
        if (!analysisModel) {
            proxyModel->setSourceModel(static_cast<AddressableItemModelI *>(nullptr));
            delete binModel;
            binModel = nullptr;
            analysisModel = new AnalysisClassesModel(this);
            proxyModel->setSourceModel(static_cast<AddressableItemModelI *>(analysisModel));
        }
        break;
    }

    qhelpers::adjustColumns(ui->treeView, 3, 0);

    // set the initial item count
    ui->quickFilterView->setItemCount(proxyModel->rowCount());

    ui->treeView->setColumnWidth(0, 200);
}

void ClassesWidget::updateActions()
{
    const bool isAnalysis = !!analysisModel;
    newClassAction.setVisible(isAnalysis);
    addMethodAction.setVisible(isAnalysis);

    bool rowIsAnalysisClass = false;
    bool rowIsAnalysisMethod = false;
    const QModelIndex index = ui->treeView->selectionModel()->currentIndex();
    if (isAnalysis && index.isValid()) {
        auto type = static_cast<ClassesModel::RowType>(index.data(ClassesModel::typeRole).toInt());
        rowIsAnalysisClass = type == ClassesModel::RowType::Class;
        rowIsAnalysisMethod = type == ClassesModel::RowType::Method;
    }

    renameClassAction.setVisible(rowIsAnalysisClass);
    deleteClassAction.setVisible(rowIsAnalysisClass);

    classesMethodsSeparator->setVisible(rowIsAnalysisClass || rowIsAnalysisMethod);

    editMethodAction.setVisible(rowIsAnalysisMethod);
    bool rowHasVTable = false;
    if (rowIsAnalysisMethod) {
        const QString className = index.parent().data(ClassesModel::nameRole).toString();
        const QString methodName = index.data(ClassesModel::nameRole).toString();
        AnalysisMethodDescription desc;
        if (Core()->getAnalysisMethod(className, methodName, &desc)) {
            if (desc.vtableOffset >= 0) {
                rowHasVTable = true;
            }
        }
    }
    seekToVTableAction.setVisible(rowHasVTable);
}

void ClassesWidget::seekToVTableActionTriggered()
{
    const QModelIndex index = ui->treeView->selectionModel()->currentIndex();
    const QString className = index.parent().data(ClassesModel::nameRole).toString();

    QList<AnalysisVTableDescription> vtables = Core()->getAnalysisClassVTables(className);
    if (vtables.isEmpty()) {
        QMessageBox::warning(this, tr("Missing VTable in class"),
                             tr("The class %1 does not have any VTable!").arg(className));
        return;
    }

    const QString methodName = index.data(ClassesModel::nameRole).toString();
    AnalysisMethodDescription desc;
    if (!Core()->getAnalysisMethod(className, methodName, &desc) || desc.vtableOffset < 0) {
        return;
    }

    Core()->seekAndShow(vtables[0].addr + desc.vtableOffset);
}

void ClassesWidget::addMethodActionTriggered()
{
    const QModelIndex index = ui->treeView->selectionModel()->currentIndex();
    if (!index.isValid()) {
        return;
    }

    QString className;
    if (index.data(ClassesModel::typeRole).toInt()
        == static_cast<int>(ClassesModel::RowType::Class)) {
        className = index.data(ClassesModel::nameRole).toString();
    } else {
        className = index.parent().data(ClassesModel::nameRole).toString();
    }

    EditMethodDialog::newMethod(className, QString(), this);
}

void ClassesWidget::editMethodActionTriggered()
{
    const QModelIndex index = ui->treeView->selectionModel()->currentIndex();
    if (!index.isValid()
        || index.data(ClassesModel::typeRole).toInt()
                != static_cast<int>(ClassesModel::RowType::Method)) {
        return;
    }
    const QString className = index.parent().data(ClassesModel::nameRole).toString();
    const QString methName = index.data(ClassesModel::nameRole).toString();
    EditMethodDialog::editMethod(className, methName, this);
}

void ClassesWidget::newClassActionTriggered()
{
    bool ok;
    const QString name = QInputDialog::getText(this, tr("Create new Class"), tr("Class Name:"),
                                               QLineEdit::Normal, QString(), &ok);
    if (ok && !name.isEmpty()) {
        Core()->createNewClass(name);
    }
}

void ClassesWidget::deleteClassActionTriggered()
{
    const QModelIndex index = ui->treeView->selectionModel()->currentIndex();
    if (!index.isValid()
        || index.data(ClassesModel::typeRole).toInt()
                != static_cast<int>(ClassesModel::RowType::Class)) {
        return;
    }
    const QString className = index.data(ClassesModel::nameRole).toString();
    if (QMessageBox::question(this, tr("Delete Class"),
                              tr("Are you sure you want to delete the class %1?").arg(className))
        != QMessageBox::StandardButton::Yes) {
        return;
    }
    Core()->deleteClass(className);
}

void ClassesWidget::renameClassActionTriggered()
{
    const QModelIndex index = ui->treeView->selectionModel()->currentIndex();
    if (!index.isValid()
        || index.data(ClassesModel::typeRole).toInt()
                != static_cast<int>(ClassesModel::RowType::Class)) {
        return;
    }
    const QString oldName = index.data(ClassesModel::nameRole).toString();
    bool ok;
    const QString newName =
            QInputDialog::getText(this, tr("Rename Class %1").arg(oldName), tr("Class name:"),
                                  QLineEdit::Normal, oldName, &ok);
    if (ok && !newName.isEmpty()) {
        Core()->renameClass(oldName, newName);
    }
}
