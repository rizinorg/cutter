#include "StackWidget.h"
#include "ui_StackWidget.h"
#include "common/JsonModel.h"
#include "common/Helpers.h"
#include "dialogs/EditInstructionDialog.h"

#include "core/MainWindow.h"
#include "QHeaderView"
#include "QMenu"

enum ColumnIndex {
    COLUMN_OFFSET = 0,
    COLUMN_VALUE,
    COLUMN_DESCRIPTION
};

StackWidget::StackWidget(MainWindow *main, QAction *action) :
    CutterDockWidget(main, action),
    ui(new Ui::StackWidget),
    addressableItemContextMenu(this, main)
{
    ui->setupUi(this);

    // Setup stack model
    modelStack->setHorizontalHeaderItem(COLUMN_OFFSET, new QStandardItem(tr("Offset")));
    modelStack->setHorizontalHeaderItem(COLUMN_VALUE, new QStandardItem(tr("Value")));
    modelStack->setHorizontalHeaderItem(COLUMN_DESCRIPTION, new QStandardItem(tr("Reference")));
    viewStack->setFont(Config()->getScaledFont());
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
    connect(modelStack, &QStandardItemModel::itemChanged, this, &StackWidget::onItemChanged);

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
        modelStack->setItem(i, COLUMN_OFFSET, rowOffset);
        modelStack->setItem(i, COLUMN_VALUE, rowValue);
        QJsonValue refObject = stackItem["ref"];
        if (!refObject.isUndefined()) { // check that the key exists
            QString ref = refObject.toString();
            if (ref.contains("ascii") && ref.count("-->") == 1) {
                ref = Core()->cmd("psz @ [" + addr + "]");
            }
            QStandardItem *rowRef = new QStandardItem(ref);
            rowRef->setEditable(false);
            QModelIndex cell = modelStack->index(i, COLUMN_DESCRIPTION);
            modelStack->setItem(i, COLUMN_DESCRIPTION, rowRef);
            if (refObject.toString().contains("ascii") && refObject.toString().count("-->") == 1) {
                modelStack->setData(cell, QVariant(QColor(243, 156, 17)),
                                    Qt::ForegroundRole); // orange
            } else if (ref.contains("program R X") && ref.count("-->") == 0) {
                modelStack->setData(cell, QVariant(QColor(Qt::red)),
                                    Qt::ForegroundRole);
            } else if (ref.contains("stack") && ref.count("-->") == 0) {
                modelStack->setData(cell, QVariant(QColor(Qt::cyan)),
                                    Qt::ForegroundRole);
            } else if (ref.contains("library") && ref.count("-->") == 0) {
                modelStack->setData(cell, QVariant(QColor(Qt::green)),
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
    viewStack->setFont(Config()->getScaledFont());
}

void StackWidget::onDoubleClicked(const QModelIndex &index)
{
    if (!index.isValid())
        return;
    // Check if we are clicking on the offset or value columns and seek if it is the case
    int column = index.column();
    if (column <= COLUMN_VALUE) {
        QString item = index.data().toString();
        Core()->seek(item);
        if (column == COLUMN_OFFSET) {
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
    QString offset = model->index(row, COLUMN_OFFSET).data().toString();
    EditInstructionDialog e(EDIT_NONE, this);
    e.setWindowTitle(tr("Edit stack at %1").arg(offset));

    QString oldBytes = model->index(row, COLUMN_VALUE).data().toString();
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
    if (currentIndex.column() != COLUMN_DESCRIPTION) {
        offsetString = currentIndex.data().toString();
    } else {
        offsetString = currentIndex.sibling(currentIndex.row(), COLUMN_VALUE).data().toString();
    }

    RVA offset = Core()->math(offsetString);
    addressableItemContextMenu.setTarget(offset);
    if (currentIndex.column() == COLUMN_OFFSET) {
        menuText.setText(tr("Stack position"));
    } else {
        menuText.setText(tr("Pointed memory"));
    }
}

void StackWidget::onItemChanged(QStandardItem *item)
{
    if (updatingData || item->column() != COLUMN_VALUE) {
        return;
    }
    QModelIndex index = item->index();
    int row = item->row();
    QString text = item->text();
    // Queue the update instead of performing immediately. Editing will trigger reload.
    // Performing reload while itemChanged signal is on stack would result
    // in itemView getting stuck in EditingState and preventing further edits.
    QMetaObject::invokeMethod(this, [this, index, row, text]() {
        QString offsetString = index.sibling(row, COLUMN_OFFSET).data().toString();
        bool ok = false;
        auto offset = offsetString.toULongLong(&ok, 16);
        if (ok) {
            Core()->editBytesEndian(offset, text);
        }
    }, Qt::QueuedConnection);
}
