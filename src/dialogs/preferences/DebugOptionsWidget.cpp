#include <QLabel>
#include <QFontDialog>

#include "DebugOptionsWidget.h"
#include "ui_DebugOptionsWidget.h"
#include <QComboBox>
#include "PreferencesDialog.h"

#include "utils/Helpers.h"
#include "utils/Configuration.h"

DebugOptionsWidget::DebugOptionsWidget(PreferencesDialog */*dialog*/, QWidget *parent)
    : QDialog(parent),
      ui(new Ui::DebugOptionsWidget)
{
    ui->setupUi(this);

    updateDebugPlugin();
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

void DebugOptionsWidget::on_pluginComboBox_currentIndexChanged(const QString &plugin)
{
    Core()->setDebugPlugin(plugin);
}
