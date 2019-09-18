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
    addressableItemContextMenu(this, main)
{
    ui->setupUi(this);

    // Setup stack model
    modelStack->setHorizontalHeaderItem(0, new QStandardItem(tr("Offset")));
    modelStack->setHorizontalHeaderItem(1, new QStandardItem(tr("Value")));
    modelStack->setHorizontalHeaderItem(2, new QStandardItem(tr("Reference")));
    viewStack->setFont(Config()->getFont());
    viewStack->setModel(modelStack);
    viewStack->verticalHeader()->hide();
    viewStack->setSortingEnabled(true);
    viewStack->setHorizontalScrollMode(QAbstractItemView::ScrollPerPixel);
    ui->verticalLayout->addWidget(viewStack);

    seekAction = new QAction(tr("Seek to this offset"), this);
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
    connect(seekAction, &QAction::triggered, this, &StackWidget::seekOffset);
    connect(editAction, &QAction::triggered, this, &StackWidget::editStack);
    connect(viewStack->selectionModel(), &QItemSelectionModel::currentChanged, this, &StackWidget::onCurrentChanged);
    connect(modelStack, &QStandardItemModel::itemChanged, this, &StackWidget::onItemChanged);

    addressableItemContextMenu.addAction(editAction);
    addActions(addressableItemContextMenu.actions());
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
    updatingData = true;
    QJsonArray stackValues = Core()->getStack().array();
    int i = 0;
    for (const QJsonValue &value : stackValues) {
        QJsonObject stackItem = value.toObject();
        QString addr = RAddressString(stackItem["addr"].toVariant().toULongLong());
        QString valueStack = RAddressString(stackItem["value"].toVariant().toULongLong());
        QStandardItem *rowOffset = new QStandardItem(addr);
        rowOffset->setEditable(false);
        QStandardItem *rowValue = new QStandardItem(valueStack);
        modelStack->setItem(i, 0, rowOffset);
        modelStack->setItem(i, 1, rowValue);
        QJsonValue refObject = stackItem["ref"];
        if (!refObject.isUndefined()) { // check that the key exists
            QString ref = refObject.toString();
            if (ref.contains("ascii") && ref.count("-->") == 1) {
                ref = Core()->cmd("psz @ [" + addr + "]");
            }
            QStandardItem *rowRef = new QStandardItem(ref);
            modelStack->setItem(i, 2, rowRef);
            if (refObject.toString().contains("ascii") && refObject.toString().count("-->") == 1) {
                modelStack->setData(modelStack->index(i, 2, QModelIndex()), QVariant(QColor(243, 156, 17)),
                                    Qt::ForegroundRole); // orange
            } else if (ref.contains("program R X") && ref.count("-->") == 0) {
                modelStack->setData(modelStack->index(i, 2, QModelIndex()), QVariant(QColor(Qt::red)),
                                    Qt::ForegroundRole);
            } else if (ref.contains("stack") && ref.count("-->") == 0) {
                modelStack->setData(modelStack->index(i, 2, QModelIndex()), QVariant(QColor(Qt::cyan)),
                                    Qt::ForegroundRole);
            } else if (ref.contains("library") && ref.count("-->") == 0) {
                modelStack->setData(modelStack->index(i, 2, QModelIndex()), QVariant(QColor(Qt::green)),
                                    Qt::ForegroundRole);
            }
        }
        i++;
    }
    viewStack->setModel(modelStack);
    viewStack->resizeColumnsToContents();
    updatingData = false;
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
    if (index.column() <= 1) {
        QString item = index.data().toString();
        Core()->seekAndShow(item);
    }
}

void StackWidget::customMenuRequested(QPoint pos)
{
    addressableItemContextMenu.exec(viewStack->viewport()->mapToGlobal(pos));
}

void StackWidget::seekOffset()
{
    QString offset = viewStack->selectionModel()->currentIndex().data().toString();
    Core()->seekAndShow(offset);
}

void StackWidget::editStack()
{
    bool ok;
    int row = viewStack->selectionModel()->currentIndex().row();
    QString offset = viewStack->selectionModel()->currentIndex().sibling(row, 0).data().toString();
    EditInstructionDialog e(EDIT_NONE, this);
    e.setWindowTitle(tr("Edit stack at %1").arg(offset));

    QString oldBytes = viewStack->selectionModel()->currentIndex().sibling(row, 1).data().toString();
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
    QString offsetString = viewStack->selectionModel()->currentIndex().data().toString();
    RVA offset = Core()->math(offsetString);
    addressableItemContextMenu.setTarget(offset);
}

void StackWidget::onItemChanged(QStandardItem *item)
{
    if (updatingData || item->column() != 1) {
        return;
    }
    QString offsetString = item->index().sibling(item->row(), 0).data().toString();
    bool ok = false;
    auto offset = offsetString.toULongLong(&ok, 16);
    if (ok) {
        Core()->editBytesEndian(offset, item->text());
    }
}
