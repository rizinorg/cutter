#include "RegistersWidget.h"
#include "ui_RegistersWidget.h"
#include "utils/JsonModel.h"

#include "MainWindow.h"
#include "QPushButton"

RegistersWidget::RegistersWidget(MainWindow *main, QAction *action) :
    CutterDockWidget(main, action),
    ui(new Ui::RegistersWidget)
{
    ui->setupUi(this);

    // setup register layout
    ui->verticalLayout->addLayout(registerLayout);

    buttonSetRegisters = new QPushButton("Set registers", this);
    connect(buttonSetRegisters, &QPushButton::clicked, this, &RegistersWidget::handleButton);

    ui->verticalLayout->addWidget(buttonSetRegisters);
    connect(Core(), &CutterCore::refreshAll, this, &RegistersWidget::updateContents);
    connect(Core(), &CutterCore::seekChanged, this, &RegistersWidget::updateContents);
}

RegistersWidget::~RegistersWidget() {}

void RegistersWidget::updateContents()
{
    setRegisterGrid();
}

 void RegistersWidget::handleButton()
 {
    int j = 0;
    int i = 0;
    int col = 0;
    for (j = 0; j<registerLen; j++) {
        QWidget *regName = registerLayout->itemAtPosition(i, col)->widget();
        QWidget *regValue = registerLayout->itemAtPosition(i, col + 1)->widget();
        QLabel *regLabel = qobject_cast<QLabel *>(regName);
        QLineEdit *regLine = qobject_cast<QLineEdit *>(regValue);
        QString regNameString = regLabel->text();
        QString regValueString = regLine->text();
        Core()->setRegister(regNameString, regValueString);
        i++;
        if (i >= registerLen/numCols + 1) {
            i = 0;
            col += 2;
        }
    }
    setRegisterGrid();
 }

void RegistersWidget::setRegisterGrid()
{
    int i = 0;
    int col = 0;
    QString regValue;
    QLabel *registerLabel;
    QLineEdit *registerEditValue;
    QJsonObject registerValues = Core()->getRegisterValues().object();
    QStringList registerNames = registerValues.keys();
    registerLen = registerNames.size();
    for (const QString &key : registerNames) {
        regValue = RAddressString(registerValues[key].toVariant().toULongLong());
        // check if we already filled this grid space with label/value
        if (!registerLayout->itemAtPosition(i, col)) {
            registerLabel = new QLabel;
            registerLabel->setStyleSheet("font-weight: bold; font-family: mono");
            registerEditValue = new QLineEdit;
            QFont font = registerEditValue->font();
            font.setWeight(QFont::Monospace);
            registerEditValue->setFont(font);
            // add label and register value to grid
            registerLayout->addWidget(registerLabel, i, col);
            registerLayout->addWidget(registerEditValue, i, col + 1);
        }
        else {
            QWidget *regNameWidget = registerLayout->itemAtPosition(i, col)->widget();
            QWidget *regValueWidget = registerLayout->itemAtPosition(i, col + 1)->widget();
            registerLabel = qobject_cast<QLabel *>(regNameWidget);
            registerEditValue = qobject_cast<QLineEdit *>(regValueWidget);
        }
        // decide to highlight QLine Box in case of change of register value
        if (regValue != registerEditValue->text() && registerEditValue->text() != 0) {
            registerEditValue->setStyleSheet("QLineEdit {border: 1px solid green;} QLineEdit:hover { border: 1px solid #3daee9; color: #eff0f1;}");
        }
        else {
            // reset stylesheet
            registerEditValue->setStyleSheet("");
        }
        // define register label and value
        registerLabel->setText(key);
        registerEditValue->setPlaceholderText(regValue);
        registerEditValue->setText(regValue);
        i++;
        // decide if we should change column
        if (i >= registerLen/numCols + 1) {
            i = 0;
            col += 2;
        }
    }
}
