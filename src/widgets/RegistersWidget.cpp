#include "RegistersWidget.h"
#include "ui_RegistersWidget.h"
#include "utils/JsonModel.h"

#include "MainWindow.h"

RegistersWidget::RegistersWidget(MainWindow *main, QAction *action) :
    CutterDockWidget(main, action),
    ui(new Ui::RegistersWidget)
{
    ui->setupUi(this);

    // setup register layout
    registerLayout->setVerticalSpacing(0);
    ui->verticalLayout->addLayout(registerLayout);

    connect(Core(), &CutterCore::refreshAll, this, &RegistersWidget::updateContents);
    connect(Core(), &CutterCore::registersChanged, this, &RegistersWidget::updateContents);
}

RegistersWidget::~RegistersWidget() {}

void RegistersWidget::updateContents()
{
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
    QJsonObject registerRefs = Core()->getRegisterJson();
    QStringList registerNames = registerValues.keys();
    registerLen = registerValues.size();
    for (const QString &key : registerNames) {
        regValue = RAddressString(registerValues[key].toVariant().toULongLong());
        // check if we already filled this grid space with label/value
        if (!registerLayout->itemAtPosition(i, col)) {
            registerLabel = new QLabel;
            registerLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
            registerLabel->setMaximumWidth(60);
            registerLabel->setStyleSheet("font-weight: bold; font-family: mono;");
            registerEditValue = new QLineEdit;
            registerEditValue->setFixedWidth(140);
            registerEditValue->setFont(Config()->getFont());
            // add label and register value to grid
            registerLayout->addWidget(registerLabel, i, col);
            registerLayout->addWidget(registerEditValue, i, col + 1);
            connect(registerEditValue, &QLineEdit::editingFinished, [=]() {
                QString regNameString = registerLabel->text();
                QString regValueString = registerEditValue->text();
                Core()->setRegister(regNameString, regValueString);
                printf("dr %s %s\n", regNameString.toLocal8Bit().constData(),
                       regValueString.toLocal8Bit().constData());
            });
        } else {
            QWidget *regNameWidget = registerLayout->itemAtPosition(i, col)->widget();
            QWidget *regValueWidget = registerLayout->itemAtPosition(i, col + 1)->widget();
            registerLabel = qobject_cast<QLabel *>(regNameWidget);
            registerEditValue = qobject_cast<QLineEdit *>(regValueWidget);
        }
        // decide to highlight QLine Box in case of change of register value
        if (regValue != registerEditValue->text() && registerEditValue->text() != 0) {
            registerEditValue->setStyleSheet("QLineEdit {border: 1px solid green;} QLineEdit:hover { border: 1px solid #3daee9; color: #eff0f1;}");
        } else {
            // reset stylesheet
            registerEditValue->setStyleSheet("");
        }
        // define register label and value
        registerLabel->setText(key);
        if (registerRefs.contains(key)) {
            // add register references to tooltips
            QString reference = registerRefs[key].toObject()["ref"].toString();
            registerLabel->setToolTip(reference);
            registerEditValue->setToolTip(reference);
        }
        registerEditValue->setPlaceholderText(regValue);
        registerEditValue->setText(regValue);
        i++;
        // decide if we should change column
        if (i >= registerLen / numCols + 1) {
            i = 0;
            col += 2;
        }
    }
}
