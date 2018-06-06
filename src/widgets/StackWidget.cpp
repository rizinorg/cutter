#include "StackWidget.h"
#include "ui_StackWidget.h"
#include "utils/JsonModel.h"
#include "utils/Helpers.h"

#include "MainWindow.h"
#include "QHeaderView"

StackWidget::StackWidget(MainWindow *main, QAction *action) :
    CutterDockWidget(main, action),
    ui(new Ui::StackWidget)
{
    ui->setupUi(this);

    // setup stack model
    modelStack->setHorizontalHeaderItem(0, new QStandardItem(tr("Offset")));
    modelStack->setHorizontalHeaderItem(1, new QStandardItem(tr("Value")));
    modelStack->setHorizontalHeaderItem(2, new QStandardItem(tr("Reference")));
    viewStack->setStyleSheet("QTableView {font-family: mono}");
    viewStack->setModel(modelStack);
    viewStack->verticalHeader()->hide();
    viewStack->setSortingEnabled(true);
    ui->verticalLayout->addWidget(viewStack);

    connect(Core(), &CutterCore::refreshAll, this, &StackWidget::updateContents);
    connect(Core(), &CutterCore::seekChanged, this, &StackWidget::updateContents);
}

StackWidget::~StackWidget() {}

void StackWidget::updateContents()
{
    setStackGrid();
}

void StackWidget::setStackGrid()
{
    QJsonArray stackValues = Core()->getStack().array();
    int i = 0;
    for (QJsonValueRef value : stackValues) {
        QJsonObject stackItem = value.toObject();
        QString addr = RAddressString(stackItem["addr"].toVariant().toULongLong());
        QString valueStack = RAddressString(stackItem["value"].toVariant().toULongLong());
        QStandardItem *rowOffset = new QStandardItem(addr);
        QStandardItem *rowValue = new QStandardItem(valueStack);
        modelStack->setItem(i, 0, rowOffset);
        modelStack->setItem(i, 1, rowValue);
        QJsonValue refObject = stackItem["ref"];
        if (!refObject.isUndefined()) { // check that the key exists
            QString ref = refObject.toString();
            QStandardItem *rowRef = new QStandardItem(ref);
            modelStack->setItem(i, 2, rowRef);
        }
        i++;
    }
    viewStack->setModel(modelStack);
    viewStack->resizeColumnsToContents();;
}
