#include "ClassesWidget.h"
#include "core/MainWindow.h"
#include "ui_ClassesWidget.h"
#include "common/Helpers.h"
#include "common/SvgIconEngine.h"
#include "dialogs/EditMethodDialog.h"
#include "dialogs/RenameDialog.h"

#include <QList>
#include <QMenu>
#include <QMouseEvent>

QVariant ClassesModel::headerData(int section, Qt::Orientation, int role) const
{
    switch (role) {
    case Qt::DisplayRole:
        switch (section) {
        case NAME:
            return tr("Name");
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



BinClassesModel::BinClassesModel(QObject *parent)
    : ClassesModel(parent)
{
}

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

    return createIndex(row, column, (quintptr)parent.row() + 1); // sub-nodes have id = class index + 1
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

        if (index.row() >= cls->baseClasses.length() + cls->methods.length() + cls->fields.length()) {
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
                return meth->addr == RVA_INVALID ? QString() : RAddressString(meth->addr);
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
                return field->addr == RVA_INVALID ? QString() : RAddressString(field->addr);
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
    }
    else {
        switch (role) {
        case Qt::DisplayRole:
            switch (index.column()) {
            case NAME:
                return cls->name;
            case TYPE:
                return tr("class");
            case OFFSET:
                return cls->addr == RVA_INVALID ? QString() : RAddressString(cls->addr);
            case VTABLE:
                return cls->vtableAddr == RVA_INVALID ? QString() : RAddressString(cls->vtableAddr);
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


AnalClassesModel::AnalClassesModel(CutterDockWidget *parent)
    : ClassesModel(parent), attrs(new QMap<QString, QVector<Attribute>>)
{
    // Just use a simple refresh deferrer. If an event was triggered in the background, simply refresh everything later.
    refreshDeferrer = parent->createRefreshDeferrer([this]() {
        this->refreshAll();
    });

    connect(Core(), &CutterCore::refreshAll, this, &AnalClassesModel::refreshAll);
    connect(Core(), &CutterCore::codeRebased, this, &AnalClassesModel::refreshAll);
    connect(Core(), &CutterCore::classNew, this, &AnalClassesModel::classNew);
    connect(Core(), &CutterCore::classDeleted, this, &AnalClassesModel::classDeleted);
    connect(Core(), &CutterCore::classRenamed, this, &AnalClassesModel::classRenamed);
    connect(Core(), &CutterCore::classAttrsChanged, this, &AnalClassesModel::classAttrsChanged);

    refreshAll();
}

void AnalClassesModel::refreshAll()
{
    if (!refreshDeferrer->attemptRefresh(nullptr)) {
        return;
    }

    beginResetModel();
    attrs->clear();
    classes = Core()->getAllAnalClasses(true); // must be sorted
    endResetModel();
}

void AnalClassesModel::classNew(const QString &cls)
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

void AnalClassesModel::classDeleted(const QString &cls)
{
    if (!refreshDeferrer->attemptRefresh(nullptr)) {
        return;
    }

    // find the position using binary search and remove the row
    auto it = std::lower_bound(classes.begin(), classes.end(), cls);
    if(it == classes.end() || *it != cls) {
        return;
    }
    int index = it - classes.begin();
    beginRemoveRows(QModelIndex(), index, index);
    classes.erase(it);
    endRemoveRows();
}

void AnalClassesModel::classRenamed(const QString &oldName, const QString &newName)
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

void AnalClassesModel::classAttrsChanged(const QString &cls)
{
    if (!refreshDeferrer->attemptRefresh(nullptr)) {
        return;
    }

    auto it = std::lower_bound(classes.begin(), classes.end(), cls);
    if(it == classes.end() || *it != cls) {
        return;
    }
    QPersistentModelIndex persistentIndex = QPersistentModelIndex(index(it - classes.begin(), 0));
    layoutAboutToBeChanged({persistentIndex});
    attrs->remove(cls);
    layoutChanged({persistentIndex});
}

const QVector<AnalClassesModel::Attribute> &AnalClassesModel::getAttrs(const QString &cls) const
{
    auto it = attrs->find(cls);
    if(it != attrs->end()) {
        return it.value();
    }

    QVector<AnalClassesModel::Attribute> clsAttrs;
    QList<AnalBaseClassDescription> bases = Core()->getAnalClassBaseClasses(cls);
    QList<AnalMethodDescription> meths = Core()->getAnalClassMethods(cls);
    QList<AnalVTableDescription> vtables = Core()->getAnalClassVTables(cls);
    clsAttrs.reserve(bases.size() + meths.size() + vtables.size());

    for(const AnalBaseClassDescription &base : bases) {
        clsAttrs.push_back(Attribute(Attribute::Type::Base, QVariant::fromValue(base)));
    }

    for(const AnalVTableDescription &vtable : vtables) {
        clsAttrs.push_back(Attribute(Attribute::Type::VTable, QVariant::fromValue(vtable)));
    }

    for(const AnalMethodDescription &meth : meths) {
        clsAttrs.push_back(Attribute(Attribute::Type::Method, QVariant::fromValue(meth)));
    }

    return attrs->insert(cls, clsAttrs).value();
}

QModelIndex AnalClassesModel::index(int row, int column, const QModelIndex &parent) const
{
    if (!parent.isValid()) {
        return createIndex(row, column, (quintptr)0); // root function nodes have id = 0
    }

    return createIndex(row, column, (quintptr)parent.row() + 1); // sub-nodes have id = class index + 1
}

QModelIndex AnalClassesModel::parent(const QModelIndex &index) const
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

int AnalClassesModel::rowCount(const QModelIndex &parent) const
{
    if (!parent.isValid()) { // root
        return classes.count();
    }

    if (parent.internalId() == 0) { // methods/fields
        return getAttrs(classes[parent.row()]).size();
    }

    return 0; // below methods/fields
}

bool AnalClassesModel::hasChildren(const QModelIndex &parent) const
{
    return !parent.isValid() || !parent.parent().isValid();
}

int AnalClassesModel::columnCount(const QModelIndex &) const
{
    return Columns::COUNT;
}

QVariant AnalClassesModel::data(const QModelIndex &index, int role) const
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
            AnalBaseClassDescription base = attr.data.value<AnalBaseClassDescription>();
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
                    return QIcon(new SvgIconEngine(QString(":/img/icons/home.svg"), QPalette::WindowText));
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
            AnalMethodDescription meth = attr.data.value<AnalMethodDescription>();
            switch (role) {
            case Qt::DisplayRole:
                switch (index.column()) {
                case NAME:
                    return meth.name;
                case TYPE:
                    return tr("method");
                case OFFSET:
                    return meth.addr == RVA_INVALID ? QString() : RAddressString(meth.addr);
                case VTABLE:
                    return meth.vtableOffset < 0 ? QString() : QString("+%1").arg(meth.vtableOffset);
                default:
                    return QVariant();
                }
            case Qt::DecorationRole:
                if (index.column() == NAME) {
                    return QIcon(new SvgIconEngine(QString(":/img/icons/fork.svg"), QPalette::WindowText));
                }
                return QVariant();
            case VTableRole:
                return QVariant::fromValue(meth.vtableOffset);
            case OffsetRole:
                return QVariant::fromValue(meth.addr);
            case NameRole:
                return meth.name;
            case TypeRole:
                return QVariant::fromValue(RowType::Method);
            default:
                return QVariant();
            }
            break;
        }
        case Attribute::Type::VTable: {
            AnalVTableDescription vtable = attr.data.value<AnalVTableDescription>();
            switch (role) {
            case Qt::DisplayRole:
                switch (index.column()) {
                case NAME:
                    return "vtable";
                case TYPE:
                    return tr("vtable");
                case OFFSET:
                    return RAddressString(vtable.addr);
                default:
                    return QVariant();
                }
            case Qt::DecorationRole:
                if (index.column() == NAME) {
                    return QIcon(new SvgIconEngine(QString(":/img/icons/list.svg"), QPalette::WindowText));
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
    : QSortFilterProxyModel(parent)
{
}

bool ClassesSortFilterProxyModel::filterAcceptsRow(int row, const QModelIndex &parent) const
{
    QModelIndex index = sourceModel()->index(row, 0, parent);
    return index.data(ClassesModel::NameRole).toString().contains(filterRegExp());
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



ClassesWidget::ClassesWidget(MainWindow *main) :
    CutterDockWidget(main),
    ui(new Ui::ClassesWidget)
{
    ui->setupUi(this);

    ui->classesTreeView->setIconSize(QSize(10, 10));

    proxy_model = new ClassesSortFilterProxyModel(this);
    ui->classesTreeView->setModel(proxy_model);
    ui->classesTreeView->sortByColumn(ClassesModel::TYPE, Qt::AscendingOrder);
    ui->classesTreeView->setContextMenuPolicy(Qt::CustomContextMenu);

    ui->classSourceCombo->setCurrentIndex(1);

    connect(ui->classSourceCombo, SIGNAL(currentIndexChanged(int)), this, SLOT(refreshClasses()));
    connect(ui->classesTreeView, &QTreeView::customContextMenuRequested, this, &ClassesWidget::showContextMenu);

    refreshClasses();
}

ClassesWidget::~ClassesWidget() {}

ClassesWidget::Source ClassesWidget::getSource()
{
    switch (ui->classSourceCombo->currentIndex()) {
    case 0:
        return Source::BIN;
    default:
        return Source::ANAL;
    }
}

void ClassesWidget::refreshClasses()
{
    switch (getSource()) {
    case Source::BIN:
        if (!bin_model) {
            proxy_model->setSourceModel(nullptr);
            delete anal_model;
            anal_model = nullptr;
            bin_model = new BinClassesModel(this);
            proxy_model->setSourceModel(bin_model);
        }
        bin_model->setClasses(Core()->getAllClassesFromBin());
        break;
    case Source::ANAL:
        if (!anal_model) {
            proxy_model->setSourceModel(nullptr);
            delete bin_model;
            bin_model = nullptr;
            anal_model = new AnalClassesModel(this);
            proxy_model->setSourceModel(anal_model);
        }
        break;
    }

    qhelpers::adjustColumns(ui->classesTreeView, 3, 0);

    ui->classesTreeView->setColumnWidth(0, 200);
}

void ClassesWidget::on_classesTreeView_doubleClicked(const QModelIndex &index)
{
    if (!index.isValid())
        return;

    QVariant offsetData = index.data(ClassesModel::OffsetRole);
    if(!offsetData.isValid()) {
        return;
    }
    RVA offset = offsetData.value<RVA>();
    Core()->seekAndShow(offset);
}

void ClassesWidget::showContextMenu(const QPoint &pt)
{
    if(!anal_model) {
        // no context menu for bin classes
        return;
    }

    QModelIndex index = ui->classesTreeView->selectionModel()->currentIndex();
    if (!index.isValid()) {
        return;
    }
    auto type = static_cast<ClassesModel::RowType>(index.data(ClassesModel::TypeRole).toInt());

    QMenu menu(ui->classesTreeView);

    menu.addAction(ui->newClassAction);

    if (type == ClassesModel::RowType::Class) {
        menu.addAction(ui->renameClassAction);
        menu.addAction(ui->deleteClassAction);
    }

    menu.addSeparator();

    menu.addAction(ui->addMethodAction);

    if (type == ClassesModel::RowType::Method) {
        menu.addAction(ui->editMethodAction);

        QString className = index.parent().data(ClassesModel::NameRole).toString();
        QString methodName = index.data(ClassesModel::NameRole).toString();
        AnalMethodDescription desc;
        if (Core()->getAnalMethod(className, methodName, &desc)) {
            if (desc.vtableOffset >= 0) {
                menu.addAction(ui->seekToVTableAction);
            }
        }
    }

    menu.exec(ui->classesTreeView->mapToGlobal(pt));
}

void ClassesWidget::on_seekToVTableAction_triggered()
{
    QModelIndex index = ui->classesTreeView->selectionModel()->currentIndex();
    QString className = index.parent().data(ClassesModel::NameRole).toString();

    QList<AnalVTableDescription> vtables = Core()->getAnalClassVTables(className);
    if (vtables.isEmpty()) {
        QMessageBox::warning(this, tr("Missing VTable in class"), tr("The class %1 does not have any VTable!").arg(className));
        return;
    }

    QString methodName = index.data(ClassesModel::NameRole).toString();
    AnalMethodDescription desc;
    if (!Core()->getAnalMethod(className, methodName, &desc) || desc.vtableOffset < 0) {
        return;
    }

    Core()->seekAndShow(vtables[0].addr + desc.vtableOffset);
}

void ClassesWidget::on_addMethodAction_triggered()
{
    QModelIndex index = ui->classesTreeView->selectionModel()->currentIndex();
    if (!index.isValid()) {
        return;
    }

    QString className;
    if (index.data(ClassesModel::TypeRole).toInt() == static_cast<int>(ClassesModel::RowType::Class)) {
        className = index.data(ClassesModel::NameRole).toString();
    } else {
        className = index.parent().data(ClassesModel::NameRole).toString();
    }

    EditMethodDialog::newMethod(className, QString(), this);
}

void ClassesWidget::on_editMethodAction_triggered()
{
    QModelIndex index = ui->classesTreeView->selectionModel()->currentIndex();
    if (!index.isValid() || index.data(ClassesModel::TypeRole).toInt() != static_cast<int>(ClassesModel::RowType::Method)) {
        return;
    }
    QString className = index.parent().data(ClassesModel::NameRole).toString();
    QString methName = index.data(ClassesModel::NameRole).toString();
    EditMethodDialog::editMethod(className, methName, this);
}


void ClassesWidget::on_newClassAction_triggered()
{
    QString name;
    if (!RenameDialog::showDialog(tr("Create new Class"), &name, tr("Class Name"), this) || name.isEmpty()) {
        return;
    }
    Core()->createNewClass(name);
}

void ClassesWidget::on_deleteClassAction_triggered()
{
    QModelIndex index = ui->classesTreeView->selectionModel()->currentIndex();
    if (!index.isValid() || index.data(ClassesModel::TypeRole).toInt() != static_cast<int>(ClassesModel::RowType::Class)) {
        return;
    }
    QString className = index.data(ClassesModel::NameRole).toString();
    if (QMessageBox::question(this, tr("Delete Class"), tr("Are you sure you want to delete the class %1?").arg(className)) != QMessageBox::StandardButton::Yes) {
        return;
    }
    Core()->deleteClass(className);
}

void ClassesWidget::on_renameClassAction_triggered()
{
    QModelIndex index = ui->classesTreeView->selectionModel()->currentIndex();
    if (!index.isValid() || index.data(ClassesModel::TypeRole).toInt() != static_cast<int>(ClassesModel::RowType::Class)) {
        return;
    }
    QString oldName = index.data(ClassesModel::NameRole).toString();
    QString newName = oldName;
    if (!RenameDialog::showDialog(tr("Rename Class %1").arg(oldName), &newName, tr("Class Name"), this) || newName.isEmpty()) {
        return;
    }
    Core()->renameClass(oldName, newName);
}
