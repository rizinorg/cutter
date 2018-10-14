#include "DebugOptionsWidget.h"
#include "ui_DebugOptionsWidget.h"
#include <QLabel>
#include <QTimer>
#include <QComboBox>
#include <QShortcut>
#include <QFontDialog>
#include "PreferencesDialog.h"

#include "utils/Helpers.h"
#include "utils/Configuration.h"

DebugOptionsWidget::DebugOptionsWidget(PreferencesDialog *dialog, QWidget *parent)
    : AbstractOptionWidget(parent),
      ui(new Ui::DebugOptionsWidget)
{
    Q_UNUSED(dialog);
    ui->setupUi(this);

    updateDebugPlugin();
}

DebugOptionsWidget::~DebugOptionsWidget() {}

void DebugOptionsWidget::apply()
{
    Core()->setConfig("dbg.args", currSettings.value("dbg.args"));
    Core()->setDebugPlugin(currSettings.value("debugPlugin").toString());
    Core()->setConfig("esil.stack.size", currSettings.value("esil.stack.size"));
    Core()->setConfig("esil.stack.addr", currSettings.value("esil.stack.addr"));
    Config()->setConfig("esil.breakoninvalid", currSettings.value("esil.breakoninvalid"));
    isChanged = false;
}

void DebugOptionsWidget::discard()
{
    currSettings.setValue("dbg.args", Core()->getConfig("dbg.args"));
//    currSettings.setValue("debugPlugin", Core()->getDebugPlugins());
    currSettings.setValue("esil.stack.size", Core()->getConfig("esil.stack.size"));
    currSettings.setValue("esil.stack.addr", Core()->getConfig("esil.stack.addr"));
    currSettings.setValue("esil.breakoninvalid", Config()->getConfigVar("esil.breakoninvalid"));
    isChanged = false;
}

void DebugOptionsWidget::updateDebugPlugin()
{
    ui->esilBreakOnInvalid->setChecked(Config()->getConfigBool("esil.breakoninvalid"));
    currSettings.setValue("esil.breakoninvalid", Config()->getConfigBool("esil.breakoninvalid"));
    disconnect(ui->pluginComboBox, SIGNAL(currentIndexChanged(const QString &)), this,
               SLOT(on_pluginComboBox_currentIndexChanged(const QString &)));

    QStringList plugins = Core()->getDebugPlugins();
    for (QString str : plugins)
        ui->pluginComboBox->addItem(str);

    QString plugin = Core()->getActiveDebugPlugin();
    ui->pluginComboBox->setCurrentText(plugin);

    connect(ui->pluginComboBox, SIGNAL(currentIndexChanged(const QString &)), this,
            SLOT(on_pluginComboBox_currentIndexChanged(const QString &)));

    QString debugArgs = Core()->getConfig("dbg.args");
    currSettings.setValue("dbg.args", debugArgs);
    ui->debugArgs->setText(debugArgs);
    ui->debugArgs->setPlaceholderText(debugArgs);
    connect(ui->debugArgs, &QLineEdit::editingFinished, this, &DebugOptionsWidget::updateDebugArgs);

    QString stackSize = Core()->getConfig("esil.stack.size");
    currSettings.setValue("esil.stack.size", stackSize);
    ui->stackSize->setText(stackSize);
    ui->stackSize->setPlaceholderText(stackSize);
    QString stackAddr = Core()->getConfig("esil.stack.addr");
    currSettings.setValue("esil.stack.addr", stackAddr);
    ui->stackAddr->setText(stackAddr);
    ui->stackAddr->setPlaceholderText(stackAddr);
    connect(ui->stackAddr, &QLineEdit::editingFinished, this, &DebugOptionsWidget::updateStackAddr);
    connect(ui->stackSize, &QLineEdit::editingFinished, this, &DebugOptionsWidget::updateStackSize);
}

void DebugOptionsWidget::updateDebugArgs()
{
    QString newArgs = ui->debugArgs->text();
    currSettings.setValue("dbg.args", newArgs);
    ui->debugArgs->setPlaceholderText(newArgs);
}

void DebugOptionsWidget::on_pluginComboBox_currentIndexChanged(const QString &plugin)
{
    currSettings.setValue("debugPlugin", plugin);
}

void DebugOptionsWidget::updateStackSize()
{
    QString newSize = ui->stackSize->text();
    currSettings.setValue("esil.stack.size", newSize);
    ui->stackSize->setPlaceholderText(newSize);
}

void DebugOptionsWidget::updateStackAddr()
{
    QString newAddr = ui->stackAddr->text();
    currSettings.setValue("esil.stack.addr", newAddr);
    ui->stackAddr->setPlaceholderText(newAddr);
}

void DebugOptionsWidget::on_esilBreakOnInvalid_toggled(bool checked)
{
    currSettings.setValue("esil.breakoninvalid", checked);
}
