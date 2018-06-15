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

DebugOptionsWidget::DebugOptionsWidget(PreferencesDialog */*dialog*/, QWidget *parent)
    : QDialog(parent),
      ui(new Ui::DebugOptionsWidget)
{
    ui->setupUi(this);

    updateDebugPlugin();
    setupDebugArgs();
}

DebugOptionsWidget::~DebugOptionsWidget() {}

void DebugOptionsWidget::updateDebugPlugin()
{
    disconnect(ui->pluginComboBox, SIGNAL(currentIndexChanged(const QString &)), this,
               SLOT(on_pluginComboBox_currentIndexChanged(const QString &)));

    QStringList plugins = Core()->getDebugPlugins();
    for (QString str : plugins)
        ui->pluginComboBox->addItem(str);

    QString plugin = Core()->getActiveDebugPlugin();
    ui->pluginComboBox->setCurrentText(plugin);

    connect(ui->pluginComboBox, SIGNAL(currentIndexChanged(const QString &)), this,
            SLOT(on_pluginComboBox_currentIndexChanged(const QString &)));
}

void DebugOptionsWidget::setupDebugArgs()
{
    // add Enter shortcut to confirm changes
    QShortcut *enterPress = new QShortcut(QKeySequence(Qt::Key_Return), this);
    enterPress->setContext(Qt::WidgetWithChildrenShortcut);
    connect(enterPress, &QShortcut::activated, this, &DebugOptionsWidget::updateDebugArgs);

    QString currentArgs = Core()->getConfig("dbg.args");
    ui->debugArgs->setText(currentArgs);
    ui->debugArgs->setPlaceholderText(currentArgs);
    connect(ui->updateArgsButton, &QAbstractButton::clicked, this, &DebugOptionsWidget::updateDebugArgs);
}

void DebugOptionsWidget::updateDebugArgs()
{
    QString newArgs = ui->debugArgs->text();
    Core()->setConfig("dbg.args", newArgs);
    ui->debugArgs->setText(newArgs);
    ui->debugArgs->setPlaceholderText(newArgs);
    // flash green for 200 ms
    ui->debugArgs->setStyleSheet("border: 1px solid green;");
    QTimer::singleShot(200, [this](){ ui->debugArgs->setStyleSheet("");});
}

void DebugOptionsWidget::on_pluginComboBox_currentIndexChanged(const QString &plugin)
{
    Core()->setDebugPlugin(plugin);
}
