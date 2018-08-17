
#include <QList>

#include "ClassesWidget.h"
#include "MainWindow.h"
#include "ui_ClassesWidget.h"
#include "common/Helpers.h"
#include "dialogs/EditMethodDialog.h"

#include <QMenu>


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
    if (!parent.isValid())
        return createIndex(row, column, (quintptr)0); // root function nodes have id = 0

    return createIndex(row, column, (quintptr)parent.row() + 1); // sub-nodes have id = class index + 1
}

QModelIndex BinClassesModel::parent(const QModelIndex &index) const
{
    if (!index.isValid() || index.column() != 0)
        return QModelIndex();

    if (index.internalId() == 0) // root function node
        return QModelIndex();
    else // sub-node
        return this->index((int)(index.internalId() - 1), 0);
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
    const ClassMethodDescription *meth = nullptr;
    const ClassFieldDescription *field = nullptr;
    const ClassBaseClassDescription *base = nullptr;
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
        case VTableOffsetRole:
            return QVariant::fromValue(index.parent().data(VTableOffsetRole).toULongLong() + meth->vtableOffset);
        case NameRole:
            return meth->name;
        case TypeRole:
            return QVariant::fromValue(METHOD);
        case DataRole:
            return QVariant::fromValue(*meth);
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
            return QVariant::fromValue(FIELD);
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
            return QVariant::fromValue(BASE);
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
        case VTableOffsetRole:
            return QVariant::fromValue(cls->vtableAddr);
        case NameRole:
            return cls->name;
        case TypeRole:
            return QVariant::fromValue(CLASS);
        default:
            return QVariant();
        }
    }
}


AnalClassesModel::AnalClassesModel(QObject *parent)
    : ClassesModel(parent)
{
}

void AnalClassesModel::setClasses(const QList<QString> &classes)
{
    beginResetModel();
    this->classes = classes;
    endResetModel();
}


QModelIndex AnalClassesModel::index(int row, int column, const QModelIndex &parent) const
{
    if (!parent.isValid())
        return createIndex(row, column, (quintptr)0); // root function nodes have id = 0

    return createIndex(row, column, (quintptr)parent.row() + 1); // sub-nodes have id = class index + 1
}

QModelIndex AnalClassesModel::parent(const QModelIndex &index) const
{
    if (!index.isValid() || index.column() != 0)
        return QModelIndex();

    if (index.internalId() == 0) // root function node
        return QModelIndex();
    else // sub-node
        return this->index((int)(index.internalId() - 1), 0);
}

int AnalClassesModel::rowCount(const QModelIndex &parent) const
{
    if (!parent.isValid()) { // root
        return classes.count();
    }

    if (parent.internalId() == 0) { // methods/fields
        return 0;
        // TODO const BinClassDescription *cls = &classes.at(parent.row());
        // TODO return cls->baseClasses.length() + cls->methods.length() + cls->fields.length();
    }

    return 0; // below methods/fields
}

bool AnalClassesModel::hasChildren(const QModelIndex &parent) const
{
    return true;
}

int AnalClassesModel::columnCount(const QModelIndex &) const
{
    return Columns::COUNT;
}

QVariant AnalClassesModel::data(const QModelIndex &index, int role) const
{
    QString cls;
    const ClassMethodDescription *meth = nullptr;
    const ClassFieldDescription *field = nullptr;
    const ClassBaseClassDescription *base = nullptr;
    if (index.internalId() == 0) { // class row
        if (index.row() >= classes.count()) {
            return QVariant();
        }

        cls = classes.at(index.row());
    } else { // method/field/base row
        cls = classes.at(static_cast<int>(index.internalId() - 1));

        /*if (index.row() >= cls->baseClasses.length() + cls->methods.length() + cls->fields.length()) {
            return QVariant();
        }

        if (index.row() < cls->baseClasses.length()) {
            base = &cls->baseClasses[index.row()];
        } else if (index.row() - cls->baseClasses.length() < cls->methods.length()) {
            meth = &cls->methods[index.row() - cls->baseClasses.length()];
        } else {
            field = &cls->fields[index.row() - cls->baseClasses.length() - cls->methods.length()];
        }*/
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
        case VTableOffsetRole:
            return QVariant::fromValue(index.parent().data(VTableOffsetRole).toULongLong() + meth->vtableOffset);
        case NameRole:
            return meth->name;
        case TypeRole:
            return QVariant::fromValue(METHOD);
        case DataRole:
            return QVariant::fromValue(*meth);
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
            return QVariant::fromValue(FIELD);
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
            return QVariant::fromValue(BASE);
        default:
            return QVariant();
        }
    }
    else {
        switch (role) {
        case Qt::DisplayRole:
            switch (index.column()) {
            case NAME:
                return cls;
            case TYPE:
                return tr("class");
            // TODO case OFFSET:
            // TODO     return cls->addr == RVA_INVALID ? QString() : RAddressString(cls->addr);
            // TODO case VTABLE:
            // TODO     return cls->vtableAddr == RVA_INVALID ? QString() : RAddressString(cls->vtableAddr);
            default:
                return QVariant();
            }
        // TODO case OffsetRole:
        // TODO     return QVariant::fromValue(cls->addr);
        // TODO case VTableOffsetRole:
        // TODO     return QVariant::fromValue(cls->vtableAddr);
        // TODO case NameRole:
        // TODO     return cls->name;
        case TypeRole:
            return QVariant::fromValue(CLASS);
        default:
            return QVariant();
        }
    }
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
        if (left_offset != right_offset)
            return left_offset < right_offset;
    }
    // fallthrough
    case ClassesModel::TYPE: {
        auto left_type = left.data(ClassesModel::TypeRole).value<ClassesModel::RowType>();
        auto right_type = right.data(ClassesModel::TypeRole).value<ClassesModel::RowType>();
        if (left_type != right_type)
            return left_type < right_type;
    }
    // fallthrough
    case ClassesModel::NAME:
    default:
        QString left_name = left.data(ClassesModel::NameRole).toString();
        QString right_name = right.data(ClassesModel::NameRole).toString();
        return left_name < right_name;
    }
}



ClassesWidget::ClassesWidget(MainWindow *main, QAction *action) :
    CutterDockWidget(main, action),
    ui(new Ui::ClassesWidget)
{
    ui->setupUi(this);

    proxy_model = new ClassesSortFilterProxyModel(this);
    ui->classesTreeView->setModel(nullptr);
    ui->classesTreeView->sortByColumn(ClassesModel::TYPE, Qt::AscendingOrder);
    ui->classesTreeView->setContextMenuPolicy(Qt::CustomContextMenu);

    ui->classSourceCombo->setCurrentIndex(1);

    connect(Core(), SIGNAL(refreshAll()), this, SLOT(refreshClasses()));
    connect(Core(), &CutterCore::classesChanged, this, [this]() {
        if (getSource() == Source::ANAL) {
            refreshClasses();
        }
    });
    connect(ui->classSourceCombo, SIGNAL(currentIndexChanged(int)), this, SLOT(refreshClasses()));
    connect(ui->classesTreeView, &QTreeView::customContextMenuRequested, this, &ClassesWidget::showContextMenu);
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
            ui->classesTreeView->setModel(bin_model);
            //proxy_model->setSourceModel(bin_model);
        }
        bin_model->setClasses(Core()->getAllClassesFromBin());
        break;
    case Source::ANAL:
        if (!anal_model) {
            proxy_model->setSourceModel(nullptr);
            delete bin_model;
            bin_model = nullptr;
            anal_model = new AnalClassesModel(this);
            ui->classesTreeView->setModel(anal_model);
            //proxy_model->setSourceModel(anal_model);
        }
        anal_model->setClasses(Core()->getAllClassesFromAnal());
        break;
    }

    qhelpers::adjustColumns(ui->classesTreeView, 3, 0);

    ui->classesTreeView->setColumnWidth(0, 200);
}

void ClassesWidget::on_classesTreeView_doubleClicked(const QModelIndex &index)
{
    if (!index.isValid())
        return;

    RVA offset = index.data(ClassesModel::OffsetRole).value<RVA>();
    Core()->seek(offset);
}

void ClassesWidget::showContextMenu(const QPoint &pt)
{
    QModelIndex index = ui->classesTreeView->selectionModel()->currentIndex();
    if (!index.isValid()) {
        return;
    }

    QMenu menu(ui->classesTreeView);

    QVariant vtableOffsetVariant = index.data(ClassesModel::VTableOffsetRole);
    if (vtableOffsetVariant.isValid() && vtableOffsetVariant.toULongLong() != RVA_INVALID) {
        menu.addAction(ui->seekToVTableAction);
    }

    menu.addAction(ui->addMethodAction);

    if (index.data(ClassesModel::TypeRole) == ClassesModel::METHOD) {
        menu.addAction(ui->editMethodAction);
    }

    menu.exec(ui->classesTreeView->mapToGlobal(pt));
}

void ClassesWidget::on_seekToVTableAction_triggered()
{
    RVA vtableOffset = ui->classesTreeView->selectionModel()->currentIndex()
        .data(ClassesModel::VTableOffsetRole).value<RVA>();
    if (vtableOffset != RVA_INVALID) {
        Core()->seek(vtableOffset);
    }
}

void ClassesWidget::on_addMethodAction_triggered()
{
    QModelIndex index = ui->classesTreeView->selectionModel()->currentIndex();
    if (!index.isValid()) {
        return;
    }

    QString className;
    if (index.data(ClassesModel::TypeRole).toInt() == ClassesModel::CLASS) {
        className = index.data(ClassesModel::NameRole).toString();
    } else {
        className = index.parent().data(ClassesModel::NameRole).toString();
    }

    ClassMethodDescription meth;
    meth.addr = Core()->getOffset();

    EditMethodDialog::newMethod(className, meth);
}

void ClassesWidget::on_editMethodAction_triggered()
{
    QModelIndex index = ui->classesTreeView->selectionModel()->currentIndex();
    if (!index.isValid() || index.data(ClassesModel::TypeRole).toInt() != ClassesModel::METHOD) {
        return;
    }

    QString className = index.parent().data(ClassesModel::NameRole).toString();
    ClassMethodDescription meth = index.data(ClassesModel::DataRole).value<ClassMethodDescription>();
    EditMethodDialog::editMethod(className, meth);
}
