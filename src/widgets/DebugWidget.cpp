#include "DebugWidget.h"
#include "ui_DebugWidget.h"
#include "utils/JsonModel.h"
#include "utils/Helpers.h"

#include "MainWindow.h"

#include <QStringList>
#include <QJsonObject>
#include <QJsonDocument>
#include <QLayoutItem>
#include <QString>
#include <QGridLayout>
#include <QStandardItem>
#include <QTableView>

DebugWidget::DebugWidget(MainWindow *main, QAction *action) :
    CutterDockWidget(main, action),
    ui(new Ui::DebugWidget)
{
    ui->setupUi(this);

    ui->verticalLayout->addLayout(registerLayout);

    model->setHorizontalHeaderItem(0, new QStandardItem(tr("Offset")));
    model->setHorizontalHeaderItem(1, new QStandardItem(tr("Value")));
    model->setHorizontalHeaderItem(2, new QStandardItem(tr("Reference")));
    view->setStyleSheet("QTableView { font-size: 12; font-family: mono }");
    view->setModel(model);
    ui->verticalLayout->addWidget(view);

    modelBacktrace->setHorizontalHeaderItem(0, new QStandardItem(tr("PC")));
    modelBacktrace->setHorizontalHeaderItem(1, new QStandardItem(tr("SP")));
    modelBacktrace->setHorizontalHeaderItem(2, new QStandardItem(tr("Frame Size")));
    modelBacktrace->setHorizontalHeaderItem(3, new QStandardItem(tr("Func Name")));
    modelBacktrace->setHorizontalHeaderItem(4, new QStandardItem(tr("Description")));
    viewBacktrace->setStyleSheet("QTableView { font-size: 12; font-family: mono }");
    viewBacktrace->setModel(modelBacktrace);
    ui->verticalLayout->addWidget(viewBacktrace);

    connect(Core(), SIGNAL(refreshAll()), this, SLOT(updateContents()));
    connect(Core(), SIGNAL(seekChanged(RVA)), this, SLOT(updateContents()));
}

DebugWidget::~DebugWidget() {}

void DebugWidget::updateContents()
{
    QJsonObject registerValues = Core()->getRegisterValues().object();
    QString strJson = Core()->getRegisterValues().toJson(QJsonDocument::Compact);
    int numCols = 2;
    setRegisterGrid(numCols);
    setStackGrid();
    setBacktraceGrid();
}

void DebugWidget::setRegisterGrid(int numCols)
{
    QJsonObject registerValues = Core()->getRegisterValues().object();
    QStringList registerNames = registerValues.keys();
    int len = registerNames.size();
    int i = 0;
    int col = 0;
    for (const QString &key : registerNames) {
        QLabel *registerLabel = new QLabel(key);
        registerLabel->setStyleSheet("font-weight: bold; font-family: mono");
        QLineEdit *registerEditValue = new QLineEdit;
        QFont font = registerEditValue->font();
        font.setWeight(QFont::Monospace); // or
        registerEditValue->setFont(font);
        registerLabel->setStyleSheet("font-family: mono; font-size: 15;");
        QString regValue = RAddressString(registerValues[key].toVariant().toULongLong());
        registerEditValue->setPlaceholderText(regValue);
        registerEditValue->setText(regValue);

        registerLayout->addWidget(registerLabel, i, col);
        registerLayout->addWidget(registerEditValue, i, col + 1);
        i++;
        if (i >= len/numCols) {
            i = 0;
            col += 2;
        }
    }
}

void DebugWidget::setStackGrid()
{
    QJsonArray stackValues = Core()->getStack().array();
    int i = 0;
    for (QJsonValueRef value : stackValues) {
        QJsonObject stackItem = value.toObject();
        QString addr = RAddressString(stackItem["addr"].toVariant().toULongLong());
        QString valueStack = RAddressString(stackItem["value"].toVariant().toULongLong());
        QStandardItem *rowOffset = new QStandardItem(addr);
        QStandardItem *rowValue = new QStandardItem(valueStack);
        model->setItem(i, 0, rowOffset);
        model->setItem(i, 1, rowValue);
        QJsonValue refObject = stackItem["ref"];
        if (!refObject.isUndefined()) { // check that the key exists
            QString ref = refObject.toString();
            QStandardItem *rowRef = new QStandardItem(ref);
            model->setItem(i, 2, rowRef);
        }
        i++;
    }
    view->setModel(model);
    view->resizeColumnsToContents();;
}

void DebugWidget::setBacktraceGrid()
{
    QJsonArray backtraceValues = Core()->getBacktrace().array();
    int i = 0;
    for (QJsonValueRef value : backtraceValues) {
        QJsonObject backtraceItem = value.toObject();
        QString progCounter = RAddressString(backtraceItem["pc"].toVariant().toULongLong());
        QString stackPointer = RAddressString(backtraceItem["sp"].toVariant().toULongLong());
        int frameSize = backtraceItem["frame_size"].toInt();
        QString funcName = backtraceItem["fname"].toString();
        QString desc = backtraceItem["desc"].toString();

        QStandardItem *rowPC = new QStandardItem(progCounter);
        QStandardItem *rowSP = new QStandardItem(stackPointer);
        QStandardItem *rowFrameSize = new QStandardItem(frameSize);
        QStandardItem *rowFuncName = new QStandardItem(funcName);
        QStandardItem *rowDesc = new QStandardItem(desc);

        modelBacktrace->setItem(i, 0, rowPC);
        modelBacktrace->setItem(i, 1, rowSP);
        modelBacktrace->setItem(i, 2, rowFrameSize);
        modelBacktrace->setItem(i, 3, rowFuncName);
        modelBacktrace->setItem(i, 4, rowDesc);
        i++;
    }
    viewBacktrace->setModel(modelBacktrace);
    viewBacktrace->resizeColumnsToContents();;
}