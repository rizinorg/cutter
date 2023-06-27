#include "ClassesWidget.h"
#include "core/MainWindow.h"
#include "ui_ListDockWidget.h"
#include "common/Helpers.h"
#include "common/SvgIconEngine.h"
#include "dialogs/EditMethodDialog.h"

#include <QList>
#include <QMenu>
#include <QMouseEvent>
#include <QInputDialog>
#include <QShortcut>
#include <QComboBox>

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
    QVariant v = data(index, OffsetRole);
    return v.isValid() ? v.toULongLong() : RVA_INVALID;
}

QString ClassesModel::name(const QModelIndex &index) const
{
    return data(index, NameRole).toString();
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
                return meth->addr == RVA_INVALID ? QString() : RzAddressString(meth->addr);
            case VTABLE:
                return meth->vtableOffset < 0 ? QString() : QString("+%1").arg(meth->vtableOffset);
            default:
                return QVariant();
            }
        case OffsetRole:
            return QVariant::fromValue(meth->addr);
        case NameRole:
            return meth->name;
        case TypeRole:
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
                return field->addr == RVA_INVALID ? QString() : RzAddressString(field->addr);
            default:
                return QVariant();
            }
        case OffsetRole:
            return QVariant::fromValue(field->addr);
        case NameRole:
            return field->name;
        case TypeRole:
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
        case NameRole:
            return base->name;
        case TypeRole:
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
                return cls->addr == RVA_INVALID ? QString() : RzAddressString(cls->addr);
            case VTABLE:
                return cls->vtableAddr == RVA_INVALID ? QString()
                                                      : RzAddressString(cls->vtableAddr);
            default:
                return QVariant();
            }
        case OffsetRole:
            return QVariant::fromValue(cls->addr);
        case NameRole:
            return cls->name;
        case TypeRole:
            return QVariant::fromValue(RowType::Class);
        default:
            return QVariant();
        }
    }
}

AnalysisClassesModel::AnalysisClassesModel(CutterDockWidget *parent)
    : ClassesModel(parent), attrs(new QMap<QString, QVector<Attribute>>)
{
    // Just use a simple refresh deferrer. If an event was triggered in the background, simply
    // refresh everything later.
    refreshDeferrer = parent->createRefreshDeferrer([this]() { this->refreshAll(); });

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
    int index = it - classes.begin();
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
    int index = it - classes.begin();
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
    int oldRow = oldIt - classes.begin();
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
    QPersistentModelIndex persistentIndex = QPersistentModelIndex(index(it - classes.begin(), 0));
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
    QList<AnalysisBaseClassDescription> bases = Core()->getAnalysisClassBaseClasses(cls);
    QList<AnalysisMethodDescription> meths = Core()->getAnalysisClassMethods(cls);
    QList<AnalysisVTableDescription> vtables = Core()->getAnalysisClassVTables(cls);
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

        QString cls = classes.at(index.row());
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
        case TypeRole:
            return QVariant::fromValue(RowType::Class);
        case NameRole:
            return cls;
        default:
            return QVariant();
        }
    } else { // method/field/base row
        QString cls = classes.at(static_cast<int>(index.internalId() - 1));
        const Attribute &attr = getAttrs(cls)[index.row()];

        switch (attr.type) {
        case Attribute::Type::Base: {
            AnalysisBaseClassDescription base = attr.data.value<AnalysisBaseClassDescription>();
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
            case VTableRole:
                return -1;
            case NameRole:
                return base.className;
            case TypeRole:
                return QVariant::fromValue(RowType::Base);
            default:
                return QVariant();
            }
            break;
        }
        case Attribute::Type::Method: {
            AnalysisMethodDescription meth = attr.data.value<AnalysisMethodDescription>();
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
                    return meth.addr == RVA_INVALID ? QString() : RzAddressString(meth.addr);
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
            case VTableRole:
                return QVariant::fromValue(meth.vtableOffset);
            case OffsetRole:
                return QVariant::fromValue(meth.addr);
            case NameRole:
                return meth.name;
            case RealNameRole:
                return meth.realName;
            case TypeRole:
                return QVariant::fromValue(RowType::Method);
            default:
                return QVariant();
            }
            break;
        }
        case Attribute::Type::VTable: {
            AnalysisVTableDescription vtable = attr.data.value<AnalysisVTableDescription>();
            switch (role) {
            case Qt::DisplayRole:
                switch (index.column()) {
                case NAME:
                    return "vtable";
                case TYPE:
                    return tr("vtable");
                case OFFSET:
                    return RzAddressString(vtable.addr);
                default:
                    return QVariant();
                }
            case Qt::DecorationRole:
                if (index.column() == NAME) {
                    return QIcon(new SvgIconEngine(QString(":/img/icons/list.svg"),
                                                   QPalette::WindowText));
                }
                return QVariant();
            case OffsetRole:
                return QVariant::fromValue(vtable.addr);
            case TypeRole:
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
    if (parent.isValid())
        return true;

    QModelIndex index = sourceModel()->index(row, 0, parent);
    return qhelpers::filterStringContains(index.data(ClassesModel::NameRole).toString(), this);
}

bool ClassesSortFilterProxyModel::lessThan(const QModelIndex &left, const QModelIndex &right) const
{
    switch (left.column()) {
    case ClassesModel::OFFSET: {
        RVA left_offset = left.data(ClassesModel::OffsetRole).toULongLong();
        RVA right_offset = right.data(ClassesModel::OffsetRole).toULongLong();
        if (left_offset != right_offset) {
            return left_offset < right_offset;
        }
    }
    // fallthrough
    case ClassesModel::TYPE: {
        auto left_type = left.data(ClassesModel::TypeRole).value<ClassesModel::RowType>();
        auto right_type = right.data(ClassesModel::TypeRole).value<ClassesModel::RowType>();
        if (left_type != right_type) {
            return left_type < right_type;
        }
    }
    // fallthrough
    case ClassesModel::VTABLE: {
        auto left_vtable = left.data(ClassesModel::VTableRole).toLongLong();
        auto right_vtable = right.data(ClassesModel::VTableRole).toLongLong();
        if (left_vtable != right_vtable) {
            return left_vtable < right_vtable;
        }
    }
    // fallthrough
    case ClassesModel::NAME:
    default:
        QString left_name = left.data(ClassesModel::NameRole).toString();
        QString right_name = right.data(ClassesModel::NameRole).toString();
        return QString::compare(left_name, right_name, Qt::CaseInsensitive) < 0;
    }
}

bool ClassesSortFilterProxyModel::hasChildren(const QModelIndex &parent) const
{
    return !parent.isValid() || !parent.parent().isValid();
}

ClassesWidget::ClassesWidget(MainWindow *main)
    : ListDockWidget(main),
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

    proxy_model = new ClassesSortFilterProxyModel(this);
    setModels(proxy_model);

    classSourceCombo = new QComboBox(this);
    // User an intermediate single-child layout to contain the combo box, otherwise
    // when the combo box is inserted directly, the entire vertical layout gets a
    // weird horizontal padding on macOS.
    QBoxLayout *comboLayout = new QBoxLayout(QBoxLayout::Direction::LeftToRight, nullptr);
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
        if (!bin_model) {
            proxy_model->setSourceModel(static_cast<AddressableItemModelI *>(nullptr));
            delete analysis_model;
            analysis_model = nullptr;
            bin_model = new BinClassesModel(this);
            proxy_model->setSourceModel(static_cast<AddressableItemModelI *>(bin_model));
        }
        bin_model->setClasses(Core()->getAllClassesFromBin());
        break;
    case Source::ANALYSIS:
        if (!analysis_model) {
            proxy_model->setSourceModel(static_cast<AddressableItemModelI *>(nullptr));
            delete bin_model;
            bin_model = nullptr;
            analysis_model = new AnalysisClassesModel(this);
            proxy_model->setSourceModel(static_cast<AddressableItemModelI *>(analysis_model));
        }
        break;
    }

    qhelpers::adjustColumns(ui->treeView, 3, 0);

    ui->treeView->setColumnWidth(0, 200);
}

void ClassesWidget::updateActions()
{
    bool isAnalysis = !!analysis_model;
    newClassAction.setVisible(isAnalysis);
    addMethodAction.setVisible(isAnalysis);

    bool rowIsAnalysisClass = false;
    bool rowIsAnalysisMethod = false;
    QModelIndex index = ui->treeView->selectionModel()->currentIndex();
    if (isAnalysis && index.isValid()) {
        auto type = static_cast<ClassesModel::RowType>(index.data(ClassesModel::TypeRole).toInt());
        rowIsAnalysisClass = type == ClassesModel::RowType::Class;
        rowIsAnalysisMethod = type == ClassesModel::RowType::Method;
    }

    renameClassAction.setVisible(rowIsAnalysisClass);
    deleteClassAction.setVisible(rowIsAnalysisClass);

    classesMethodsSeparator->setVisible(rowIsAnalysisClass || rowIsAnalysisMethod);

    editMethodAction.setVisible(rowIsAnalysisMethod);
    bool rowHasVTable = false;
    if (rowIsAnalysisMethod) {
        QString className = index.parent().data(ClassesModel::NameRole).toString();
        QString methodName = index.data(ClassesModel::NameRole).toString();
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
    QModelIndex index = ui->treeView->selectionModel()->currentIndex();
    QString className = index.parent().data(ClassesModel::NameRole).toString();

    QList<AnalysisVTableDescription> vtables = Core()->getAnalysisClassVTables(className);
    if (vtables.isEmpty()) {
        QMessageBox::warning(this, tr("Missing VTable in class"),
                             tr("The class %1 does not have any VTable!").arg(className));
        return;
    }

    QString methodName = index.data(ClassesModel::NameRole).toString();
    AnalysisMethodDescription desc;
    if (!Core()->getAnalysisMethod(className, methodName, &desc) || desc.vtableOffset < 0) {
        return;
    }

    Core()->seekAndShow(vtables[0].addr + desc.vtableOffset);
}

void ClassesWidget::addMethodActionTriggered()
{
    QModelIndex index = ui->treeView->selectionModel()->currentIndex();
    if (!index.isValid()) {
        return;
    }

    QString className;
    if (index.data(ClassesModel::TypeRole).toInt()
        == static_cast<int>(ClassesModel::RowType::Class)) {
        className = index.data(ClassesModel::NameRole).toString();
    } else {
        className = index.parent().data(ClassesModel::NameRole).toString();
    }

    EditMethodDialog::newMethod(className, QString(), this);
}

void ClassesWidget::editMethodActionTriggered()
{
    QModelIndex index = ui->treeView->selectionModel()->currentIndex();
    if (!index.isValid()
        || index.data(ClassesModel::TypeRole).toInt()
                != static_cast<int>(ClassesModel::RowType::Method)) {
        return;
    }
    QString className = index.parent().data(ClassesModel::NameRole).toString();
    QString methName = index.data(ClassesModel::NameRole).toString();
    EditMethodDialog::editMethod(className, methName, this);
}

void ClassesWidget::newClassActionTriggered()
{
    bool ok;
    QString name = QInputDialog::getText(this, tr("Create new Class"), tr("Class Name:"),
                                         QLineEdit::Normal, QString(), &ok);
    if (ok && !name.isEmpty()) {
        Core()->createNewClass(name);
    }
}

void ClassesWidget::deleteClassActionTriggered()
{
    QModelIndex index = ui->treeView->selectionModel()->currentIndex();
    if (!index.isValid()
        || index.data(ClassesModel::TypeRole).toInt()
                != static_cast<int>(ClassesModel::RowType::Class)) {
        return;
    }
    QString className = index.data(ClassesModel::NameRole).toString();
    if (QMessageBox::question(this, tr("Delete Class"),
                              tr("Are you sure you want to delete the class %1?").arg(className))
        != QMessageBox::StandardButton::Yes) {
        return;
    }
    Core()->deleteClass(className);
}

void ClassesWidget::renameClassActionTriggered()
{
    QModelIndex index = ui->treeView->selectionModel()->currentIndex();
    if (!index.isValid()
        || index.data(ClassesModel::TypeRole).toInt()
                != static_cast<int>(ClassesModel::RowType::Class)) {
        return;
    }
    QString oldName = index.data(ClassesModel::NameRole).toString();
    bool ok;
    QString newName = QInputDialog::getText(this, tr("Rename Class %1").arg(oldName),
                                            tr("Class name:"), QLineEdit::Normal, oldName, &ok);
    if (ok && !newName.isEmpty()) {
        Core()->renameClass(oldName, newName);
    }
}
