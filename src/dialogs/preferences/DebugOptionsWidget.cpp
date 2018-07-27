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
    : QDialog(parent),
      ui(new Ui::DebugOptionsWidget)
{
    Q_UNUSED(dialog);
    ui->setupUi(this);

    updateDebugPlugin();
}

DebugOptionsWidget::~DebugOptionsWidget() {}

void DebugOptionsWidget::updateDebugPlugin()
{
    ui->esilBreakOnInvalid->setChecked(Config()->getConfigBool("esil.breakoninvalid"));
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
    ui->debugArgs->setText(debugArgs);
    ui->debugArgs->setPlaceholderText(debugArgs);
    connect(ui->debugArgs, &QLineEdit::editingFinished, this, &DebugOptionsWidget::updateDebugArgs);

    QString stackSize = Core()->getConfig("esil.stack.size");
    ui->stackSize->setText(stackSize);
    ui->stackSize->setPlaceholderText(stackSize);
    QString stackAddr = Core()->getConfig("esil.stack.addr");
    ui->stackAddr->setText(stackAddr);
    ui->stackAddr->setPlaceholderText(stackAddr);
    connect(ui->stackAddr, &QLineEdit::editingFinished, this, &DebugOptionsWidget::updateStackAddr);
    connect(ui->stackSize, &QLineEdit::editingFinished, this, &DebugOptionsWidget::updateStackSize);
}

void DebugOptionsWidget::updateDebugArgs()
{
    QString newArgs = ui->debugArgs->text();
    Core()->setConfig("dbg.args", newArgs);
    ui->debugArgs->setPlaceholderText(newArgs);
}

void DebugOptionsWidget::on_pluginComboBox_currentIndexChanged(const QString &plugin)
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

void DebugOptionsWidget::on_esilBreakOnInvalid_toggled(bool checked)
{
    Config()->setConfig("esil.breakoninvalid", checked);
}
