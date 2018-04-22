
#include <QList>

#include "ClassesWidget.h"
#include "MainWindow.h"
#include "ui_ClassesWidget.h"
#include "utils/Helpers.h"

ClassesModel::ClassesModel(QList<ClassDescription> *classes, QObject *parent)
    : QAbstractItemModel(parent),
      classes(classes)
{
}


QModelIndex ClassesModel::index(int row, int column, const QModelIndex &parent) const
{
    if (!parent.isValid())
        return createIndex(row, column, (quintptr)0); // root function nodes have id = 0

    return createIndex(row, column, (quintptr)parent.row() + 1); // sub-nodes have id = class index + 1
}

QModelIndex ClassesModel::parent(const QModelIndex &index) const
{
    if (!index.isValid() || index.column() != 0)
        return QModelIndex();

    if (index.internalId() == 0) // root function node
        return QModelIndex();
    else // sub-node
        return this->index((int)(index.internalId() - 1), 0);
}

int ClassesModel::rowCount(const QModelIndex &parent) const
{
    if (!parent.isValid()) { // root
        return classes->count();
    }

    if (parent.internalId() == 0) { // methods/fields
        const ClassDescription *cls = &classes->at(parent.row());
        return cls->methods.length() + cls->fields.length();
    }

    return 0; // below methods/fields
}

int ClassesModel::columnCount(const QModelIndex &) const
{
    return Columns::COUNT;
}

QVariant ClassesModel::data(const QModelIndex &index, int role) const
{
    const ClassDescription *cls;
    const ClassMethodDescription *meth = nullptr;
    const ClassFieldDescription *field = nullptr;
    if (index.internalId() == 0) { // class row
        if (index.row() >= classes->count()) {
            return QVariant();
        }

        cls = &classes->at(index.row());
    } else { // method/field row
        cls = &classes->at(static_cast<int>(index.internalId() - 1));

        if (index.row() >= cls->methods.length() + cls->fields.length()) {
            return QVariant();
        }

        if (index.row() < cls->methods.length()) {
            meth = &cls->methods[index.row()];
        } else {
            field = &cls->fields[index.row() - cls->methods.length()];
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
                return RAddressString(meth->addr);
            default:
                return QVariant();
            }
        case OffsetRole:
            return QVariant::fromValue(meth->addr);
        case NameRole:
            return meth->name;
        case TypeRole:
            return QVariant::fromValue(METHOD);
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
                return RAddressString(field->addr);
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
    } else {
        switch (role) {
        case Qt::DisplayRole:
            switch (index.column()) {
            case NAME:
                return cls->name;
            case TYPE:
                return tr("class");
            case OFFSET:
                return RAddressString(cls->addr);
            default:
                return QVariant();
            }
        case OffsetRole:
            return QVariant::fromValue(cls->addr);
        case NameRole:
            return cls->name;
        case TypeRole:
            return QVariant::fromValue(CLASS);
        default:
            return QVariant();
        }
    }
}

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
        default:
            return QVariant();
        }
    default:
        return QVariant();
    }
}

void ClassesModel::beginReload()
{
    beginResetModel();
}

void ClassesModel::endReload()
{
    endResetModel();
}





ClassesSortFilterProxyModel::ClassesSortFilterProxyModel(ClassesModel *source_model,
                                                         QObject *parent)
    : QSortFilterProxyModel(parent)
{
    setSourceModel(source_model);
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

    model = new ClassesModel(&classes, this);
    proxy_model = new ClassesSortFilterProxyModel(model, this);
    ui->classesTreeView->setModel(proxy_model);
    ui->classesTreeView->sortByColumn(ClassesModel::TYPE, Qt::AscendingOrder);

    connect(Core(), SIGNAL(refreshAll()), this, SLOT(refreshClasses()));
    connect(Core(), SIGNAL(flagsChanged()), this, SLOT(flagsChanged()));
    connect(ui->classSourceCombo, SIGNAL(currentIndexChanged(int)), this, SLOT(refreshClasses()));
}

ClassesWidget::~ClassesWidget() {}

ClassesWidget::Source ClassesWidget::getSource()
{
    if (ui->classSourceCombo->currentIndex() == 1) {
        return Source::FLAGS;
    } else {
        return Source::BIN;
    }
}

void ClassesWidget::flagsChanged()
{
    if (getSource() == Source::FLAGS) {
        refreshClasses();
    }
}

void ClassesWidget::refreshClasses()
{
    model->beginReload();
    classes = getSource() == Source::BIN
              ? Core()->getAllClassesFromBin()
              : Core()->getAllClassesFromFlags();
    model->endReload();

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
