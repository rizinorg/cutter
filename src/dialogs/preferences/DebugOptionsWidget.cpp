#include "DebugOptionsWidget.h"
#include "ui_DebugOptionsWidget.h"
#include <QLabel>
#include <QTimer>
#include <QComboBox>
#include <QShortcut>
#include <QFontDialog>
#include "PreferencesDialog.h"

#include "common/Helpers.h"
#include "common/Configuration.h"

DebugOptionsWidget::DebugOptionsWidget(PreferencesDialog *dialog)
    : QDialog(dialog), ui(new Ui::DebugOptionsWidget)
{
    ui->setupUi(this);

    updateDebugPlugin();

    connect(ui->pluginComboBox, &QComboBox::currentTextChanged, this,
            &DebugOptionsWidget::onDebugPluginChanged);
}

DebugOptionsWidget::~DebugOptionsWidget() {}

void DebugOptionsWidget::updateDebugPlugin()
{
    ui->traceContinue->setChecked(Config()->getConfigBool("dbg.trace_continue"));
    connect(ui->traceContinue, &QCheckBox::toggled, this,
            [](bool checked) { Config()->setConfig("dbg.trace_continue", checked); });
    ui->esilBreakOnInvalid->setChecked(Config()->getConfigBool("esil.breakoninvalid"));
    connect(ui->esilBreakOnInvalid, &QCheckBox::toggled, this,
            [](bool checked) { Config()->setConfig("esil.breakoninvalid", checked); });

    disconnect(ui->pluginComboBox, &QComboBox::currentTextChanged, this,
               &DebugOptionsWidget::onDebugPluginChanged);

    QStringList plugins = Core()->getDebugPlugins();
    for (const QString &str : plugins)
        ui->pluginComboBox->addItem(str);

    QString plugin = Core()->getActiveDebugPlugin();
    ui->pluginComboBox->setCurrentText(plugin);

    connect(ui->pluginComboBox, &QComboBox::currentTextChanged, this,
            &DebugOptionsWidget::onDebugPluginChanged);

    QString stackSize = Core()->getConfig("esil.stack.size");
    ui->stackSize->setText(stackSize);
    ui->stackSize->setPlaceholderText(stackSize);
    QString stackAddr = Core()->getConfig("esil.stack.addr");
    ui->stackAddr->setText(stackAddr);
    ui->stackAddr->setPlaceholderText(stackAddr);
    connect(ui->stackAddr, &QLineEdit::editingFinished, this, &DebugOptionsWidget::updateStackAddr);
    connect(ui->stackSize, &QLineEdit::editingFinished, this, &DebugOptionsWidget::updateStackSize);
}

void DebugOptionsWidget::onDebugPluginChanged(const QString &plugin)
{
    Core()->setDebugPlugin(plugin);
}

void DebugOptionsWidget::updateStackSize()
{
    QString newSize = ui->stackSize->text();
    Core()->setConfig("esil.stack.size", newSize);
    ui->stackSize->setPlaceholderText(newSize);
}

void DebugOptionsWidget::updateStackAddr()
{
    QString newAddr = ui->stackAddr->text();
    Core()->setConfig("esil.stack.addr", newAddr);
    ui->stackAddr->setPlaceholderText(newAddr);
}
