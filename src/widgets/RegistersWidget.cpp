#include "RegistersWidget.h"
#include "ui_RegistersWidget.h"
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

RegistersWidget::RegistersWidget(MainWindow *main, QAction *action) :
    CutterDockWidget(main, action),
    ui(new Ui::RegistersWidget)
{
    ui->setupUi(this);

    // setup register layout
    ui->verticalLayout->addLayout(registerLayout);

    connect(Core(), SIGNAL(refreshAll()), this, SLOT(updateContents()));
    connect(Core(), SIGNAL(seekChanged(RVA)), this, SLOT(updateContents()));
}

RegistersWidget::~RegistersWidget() {}

void RegistersWidget::updateContents()
{
    int numCols = 2;
    setRegisterGrid(numCols);
}

void RegistersWidget::setRegisterGrid(int numCols)
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
        font.setWeight(QFont::Monospace);
        registerEditValue->setFont(font);
        // define register value
        QString regValue = RAddressString(registerValues[key].toVariant().toULongLong());
        registerEditValue->setPlaceholderText(regValue);
        registerEditValue->setText(regValue);
        // add label and register value
        registerLayout->addWidget(registerLabel, i, col);
        registerLayout->addWidget(registerEditValue, i, col + 1);
        i++;
        if (i >= len/numCols + 1) {
            i = 0;
            col += 2;
        }
    }
}
