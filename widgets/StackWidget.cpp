#include "StackWidget.h"
#include "ui_StackWidget.h"
#include "common/JsonModel.h"
#include "common/Helpers.h"
#include "dialogs/EditInstructionDialog.h"

#include "core/MainWindow.h"
#include "QHeaderView"
#include "QMenu"

StackWidget::StackWidget(MainWindow *main, QAction *action) :
    CutterDockWidget(main, action),
    ui(new Ui::StackWidget),
    menuText(this),
    addressableItemContextMenu(this, main)
{
    ui->setupUi(this);

    // Setup stack model
    viewStack->setFont(Config()->getFont());
    viewStack->setModel(modelStack);
    viewStack->verticalHeader()->hide();
    viewStack->setSortingEnabled(true);
    viewStack->setHorizontalScrollMode(QAbstractItemView::ScrollPerPixel);
    ui->verticalLayout->addWidget(viewStack);
    viewStack->setEditTriggers(viewStack->editTriggers() &
                               ~(QAbstractItemView::DoubleClicked | QAbstractItemView::AnyKeyPressed));

    editAction = new QAction(tr("Edit stack value..."), this);
    viewStack->setContextMenuPolicy(Qt::CustomContextMenu);

    refreshDeferrer = createRefreshDeferrer([this]() {
        updateContents();
    });

    connect(Core(), &CutterCore::refreshAll, this, &StackWidget::updateContents);
    connect(Core(), &CutterCore::seekChanged, this, &StackWidget::updateContents);
    connect(Core(), &CutterCore::stackChanged, this, &StackWidget::updateContents);
    connect(Config(), &Configuration::fontsUpdated, this, &StackWidget::fontsUpdatedSlot);
    connect(viewStack, SIGNAL(doubleClicked(const QModelIndex &)), this,
            SLOT(onDoubleClicked(const QModelIndex &)));
    connect(viewStack, SIGNAL(customContextMenuRequested(QPoint)), SLOT(customMenuRequested(QPoint)));
    connect(editAction, &QAction::triggered, this, &StackWidget::editStack);
    connect(viewStack->selectionModel(), &QItemSelectionModel::currentChanged,
            this, &StackWidget::onCurrentChanged);

    addressableItemContextMenu.addAction(editAction);
    addActions(addressableItemContextMenu.actions());

    menuText.setSeparator(true);
    qhelpers::prependQAction(&menuText, &addressableItemContextMenu);
}

StackWidget::~StackWidget() = default;

void StackWidget::updateContents()
{
    if (!refreshDeferrer->attemptRefresh(nullptr)) {
        return;
    }

    setStackGrid();
}

void StackWidget::setStackGrid()
{
    modelStack->reload();
    viewStack->resizeColumnsToContents();
}

void StackWidget::fontsUpdatedSlot()
{
    viewStack->setFont(Config()->getFont());
}

void StackWidget::onDoubleClicked(const QModelIndex &index)
{
    if (!index.isValid())
        return;
    // Check if we are clicking on the offset or value columns and seek if it is the case
    int column = index.column();
    if (column <= StackModel::ValueColumn) {
        QString item = index.data().toString();
        Core()->seek(item);
        if (column == StackModel::OffsetColumn) {
            mainWindow->showMemoryWidget(MemoryWidgetType::Hexdump);
        } else {
            Core()->showMemoryWidget();
        }
    }
}

void StackWidget::customMenuRequested(QPoint pos)
{
    addressableItemContextMenu.exec(viewStack->viewport()->mapToGlobal(pos));
}

void StackWidget::editStack()
{
    bool ok;
    int row = viewStack->selectionModel()->currentIndex().row();
    auto model = viewStack->model();
    QString offset = model->index(row, StackModel::OffsetColumn).data().toString();
    EditInstructionDialog e(EDIT_NONE, this);
    e.setWindowTitle(tr("Edit stack at %1").arg(offset));

    QString oldBytes = model->index(row, StackModel::ValueColumn).data().toString();
    e.setInstruction(oldBytes);

    if (e.exec()) {
        QString bytes = e.getInstruction();
        if (bytes != oldBytes) {
            Core()->editBytesEndian(offset.toULongLong(&ok, 16), bytes);
        }
    }
}

void StackWidget::onCurrentChanged(const QModelIndex &current, const QModelIndex &previous)
{
    Q_UNUSED(current)
    Q_UNUSED(previous)
    auto currentIndex = viewStack->selectionModel()->currentIndex();
    QString offsetString;
    if (currentIndex.column() != StackModel::DescriptionColumn) {
        offsetString = currentIndex.data().toString();
    } else {
        offsetString = currentIndex.sibling(currentIndex.row(), StackModel::ValueColumn).data().toString();
    }

    RVA offset = Core()->math(offsetString);
    addressableItemContextMenu.setTarget(offset);
    if (currentIndex.column() == StackModel::OffsetColumn) {
        menuText.setText(tr("Stack position"));
    } else {
        menuText.setText(tr("Pointed memory"));
    }
}

StackModel::StackModel(QObject *parent)
    : QAbstractTableModel(parent)
{
}

void StackModel::reload()
{
    QJsonArray stackValues = Core()->getStack().array();

    beginResetModel();
    values.clear();
    for (const QJsonValue &value : stackValues) {
        QJsonObject stackItem = value.toObject();
        Item item;

        item.offset = stackItem["addr"].toVariant().toULongLong();
        item.value = RAddressString(stackItem["value"].toVariant().toULongLong());


        QJsonValue refObject = stackItem["ref"];
        if (!refObject.isUndefined()) { // check that the key exists
            QString ref = refObject.toString();
            if (ref.contains("ascii") && ref.count("-->") == 1) {
                ref = Core()->cmdj(QString("pszj @ [%1]").arg(item.offset)).object().value("string").toString();
            }
            item.description = ref;


            if (refObject.toString().contains("ascii") && refObject.toString().count("-->") == 1) {
                item.descriptionColor = QVariant(QColor(243, 156, 17));
            } else if (ref.contains("program R X") && ref.count("-->") == 0) {
                item.descriptionColor = QVariant(QColor(Qt::red));
            } else if (ref.contains("stack") && ref.count("-->") == 0) {
                item.descriptionColor = QVariant(QColor(Qt::cyan));
            } else if (ref.contains("library") && ref.count("-->") == 0) {
                item.descriptionColor = QVariant(QColor(Qt::green));
            }
        }
        values.push_back(item);
    }
    endResetModel();
}

int StackModel::rowCount(const QModelIndex &) const
{
    return this->values.size();
}

int StackModel::columnCount(const QModelIndex &) const
{
    return ColumnCount;
}

QVariant StackModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() >= values.count())
        return QVariant();

    const auto &item = values.at(index.row());

    switch (role) {
    case Qt::DisplayRole:
        switch (index.column()) {
        case OffsetColumn:
            return RAddressString(item.offset);
        case ValueColumn:
            return item.value;
        case DescriptionColumn:
            return item.description;
        default:
            return QVariant();
        }
    case Qt::ForegroundRole:
        switch (index.column()) {
        case DescriptionColumn:
            return item.descriptionColor;
        default:
            return QVariant();
        }
    case StackDescriptionRole:
        return QVariant::fromValue(item);
    default:
        return QVariant();
    }
}

QVariant StackModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    Q_UNUSED(orientation);
    switch (role) {
    case Qt::DisplayRole:
        switch (section) {
        case OffsetColumn:
            return tr("Offset");
        case ValueColumn:
            return tr("Value");
        case DescriptionColumn:
            return tr("Reference");
        default:
            return QVariant();
        }
    default:
        return QVariant();
    }
}

bool StackModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    if (role != Qt::EditRole || index.column() != ValueColumn) {
        return false;
    }

    auto currentData = data(index, StackDescriptionRole);
    if (!currentData.canConvert<StackModel::Item>()) {
        return false;
    }
    auto currentItem = currentData.value<StackModel::Item>();

    Core()->editBytesEndian(currentItem.offset, value.toString());
    return true;
}

Qt::ItemFlags StackModel::flags(const QModelIndex &index) const
{
    switch (index.column()) {
    case ValueColumn:
        return QAbstractTableModel::flags(index) | Qt::ItemIsEditable;
    default:
        return QAbstractTableModel::flags(index);
    }
}
