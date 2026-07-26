#include "DebugOptionsWidget.h"

#include "PreferencesDialog.h"
#include "common/Configuration.h"
#include "common/Helpers.h"
#include "ui_DebugOptionsWidget.h"

#include <QComboBox>
#include <QFontDialog>
#include <QLabel>
#include <QShortcut>
#include <QTimer>

DebugOptionsWidget::DebugOptionsWidget(PreferencesDialog *dialog)
    : QDialog(dialog), ui(new Ui::DebugOptionsWidget)
{
    ui->setupUi(this);

    connect(ui->traceContinue, &QCheckBox::toggled, this, [this](bool checked) {
        Config()->setConfig("dbg.trace_continue", checked);
        this->debugOptionsChanged();
    });

    connect(ui->esilBreakOnInvalid, &QCheckBox::toggled, this, [this](bool checked) {
        Config()->setConfig("esil.breakoninvalid", checked);
        this->debugOptionsChanged();
    });

    connect(ui->stackAddr, &QLineEdit::editingFinished, this, &DebugOptionsWidget::updateStackAddr);
    connect(ui->stackSize, &QLineEdit::editingFinished, this, &DebugOptionsWidget::updateStackSize);

    updateDebugPlugin();

    connect(ui->pluginComboBox, &QComboBox::currentTextChanged, this,
            &DebugOptionsWidget::onDebugPluginChanged);

    connect(Core(), &CutterCore::debugOptionsChanged, this, &DebugOptionsWidget::updateDebugPlugin);
}

DebugOptionsWidget::~DebugOptionsWidget() {}

void DebugOptionsWidget::updateDebugPlugin()
{
    qhelpers::setCheckedWithoutSignals(ui->traceContinue,
                                       Config()->getConfigBool("dbg.trace_continue"));

    qhelpers::setCheckedWithoutSignals(ui->esilBreakOnInvalid,
                                       Config()->getConfigBool("esil.breakoninvalid"));

    disconnect(ui->pluginComboBox, &QComboBox::currentTextChanged, this,
               &DebugOptionsWidget::onDebugPluginChanged);

    ui->pluginComboBox->blockSignals(true);
    ui->pluginComboBox->clear();
    const QStringList plugins = Core()->getDebugPlugins();
    for (const QString &str : plugins) {
        ui->pluginComboBox->addItem(str);
    }

    const QString plugin = Core()->getActiveDebugPlugin();
    ui->pluginComboBox->setCurrentText(plugin);
    ui->pluginComboBox->blockSignals(false);

    connect(ui->pluginComboBox, &QComboBox::currentTextChanged, this,
            &DebugOptionsWidget::onDebugPluginChanged);

    const QString stackSize = Core()->getConfig("esil.stack.size");
    ui->stackSize->setText(stackSize);
    ui->stackSize->setPlaceholderText(stackSize);
    const QString stackAddr = Core()->getConfig("esil.stack.addr");
    ui->stackAddr->setText(stackAddr);
    ui->stackAddr->setPlaceholderText(stackAddr);
}

void DebugOptionsWidget::onDebugPluginChanged(const QString &plugin)
{
    Core()->setDebugPlugin(plugin);

    this->debugOptionsChanged();
}

void DebugOptionsWidget::updateStackSize()
{
    const QString newSize = ui->stackSize->text();
    Core()->setConfig("esil.stack.size", newSize);
    ui->stackSize->setPlaceholderText(newSize);

    this->debugOptionsChanged();
}

void DebugOptionsWidget::updateStackAddr()
{
    const QString newAddr = ui->stackAddr->text();
    Core()->setConfig("esil.stack.addr", newAddr);
    ui->stackAddr->setPlaceholderText(newAddr);

    this->debugOptionsChanged();
}

void DebugOptionsWidget::debugOptionsChanged() const
{
    disconnect(Core(), &CutterCore::debugOptionsChanged, this,
               &DebugOptionsWidget::updateDebugPlugin);
    Core()->triggerDebugOptionsChanged();
    connect(Core(), &CutterCore::debugOptionsChanged, this, &DebugOptionsWidget::updateDebugPlugin);
}
