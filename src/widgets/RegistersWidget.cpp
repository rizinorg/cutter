#include "RegistersWidget.h"
#include "ui_RegistersWidget.h"
#include "common/JsonModel.h"

#include "core/MainWindow.h"

#include <QCollator>
#include <QLabel>
#include <QLineEdit>

RegistersWidget::RegistersWidget(MainWindow *main) :
    CutterDockWidget(main),
    ui(new Ui::RegistersWidget),
    addressContextMenu(this, main)
{
    ui->setupUi(this);

    // setup register layout
    registerLayout->setVerticalSpacing(0);
    registerLayout->setAlignment(Qt::AlignLeft |  Qt::AlignTop) ;
    ui->verticalLayout->addLayout(registerLayout);

    refreshDeferrer = createRefreshDeferrer([this]() {
        updateContents();
    });

    connect(Core(), &CutterCore::refreshAll, this, &RegistersWidget::updateContents);
    connect(Core(), &CutterCore::registersChanged, this, &RegistersWidget::updateContents);

    // Hide shortcuts because there is no way of selecting an item and triger them
    for (auto &action : addressContextMenu.actions()) {
        action->setShortcut(QKeySequence());
        // setShortcutVisibleInContextMenu(false) doesn't work
    }
}

RegistersWidget::~RegistersWidget() = default;

void RegistersWidget::updateContents()
{
    if (!refreshDeferrer->attemptRefresh(nullptr)) {
        return;
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
    const auto registerRefs = Core()->getRegisterRefValues();

    registerLen = registerRefs.size();
    for (auto &reg : registerRefs) {
        regValue = "0x" + reg.value;
        // check if we already filled this grid space with label/value
        if (!registerLayout->itemAtPosition(i, col)) {
            registerLabel = new QLabel;
            registerLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);
            registerLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
            registerLabel->setMaximumWidth(60);
            registerLabel->setStyleSheet("font-weight: bold; font-family: mono;");
            registerEditValue = new QLineEdit;
            registerEditValue->setMaximumWidth(140);
            registerEditValue->setFont(Config()->getFont());
            registerLabel->setContextMenuPolicy(Qt::CustomContextMenu);
            connect(registerLabel, &QWidget::customContextMenuRequested, this, [this, registerEditValue, registerLabel](QPoint p){
                openContextMenu(registerLabel->mapToGlobal(p), registerEditValue->text());
            });
            registerEditValue->setContextMenuPolicy(Qt::CustomContextMenu);
            connect(registerEditValue, &QWidget::customContextMenuRequested, this, [this, registerEditValue](QPoint p){
                openContextMenu(registerEditValue->mapToGlobal(p), registerEditValue->text());
            });
            // add label and register value to grid
            registerLayout->addWidget(registerLabel, i, col);
            registerLayout->addWidget(registerEditValue, i, col + 1);
            connect(registerEditValue, &QLineEdit::editingFinished, [ = ]() {
                QString regNameString = registerLabel->text();
                QString regValueString = registerEditValue->text();
                Core()->setRegister(regNameString, regValueString);
            });
        } else {
            QWidget *regNameWidget = registerLayout->itemAtPosition(i, col)->widget();
            QWidget *regValueWidget = registerLayout->itemAtPosition(i, col + 1)->widget();
            registerLabel = qobject_cast<QLabel *>(regNameWidget);
            registerEditValue = qobject_cast<QLineEdit *>(regValueWidget);
        }
        // decide to highlight QLine Box in case of change of register value
        if (regValue != registerEditValue->text() && registerEditValue->text() != 0) {
            registerEditValue->setStyleSheet("border: 1px solid green;");
        } else {
            // reset stylesheet
            registerEditValue->setStyleSheet("");
        }
        // define register label and value
        registerLabel->setText(reg.name);

        registerLabel->setToolTip(reg.ref);
        registerEditValue->setToolTip(reg.ref);

        registerEditValue->setPlaceholderText(regValue);
        registerEditValue->setText(regValue);
        i++;
        // decide if we should change column
        if (i >= (registerLen + numCols - 1) / numCols) {
            i = 0;
            col += 2;
        }
    }
}

void RegistersWidget::openContextMenu(QPoint point, QString address)
{
    addressContextMenu.setTarget(address.toULongLong(nullptr, 16));
    addressContextMenu.exec(point);
}
