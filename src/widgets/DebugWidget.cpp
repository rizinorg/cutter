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