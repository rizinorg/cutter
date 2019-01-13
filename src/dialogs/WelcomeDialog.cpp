#include "MainWindow.h"
#include "CutterConfig.h"

#include "common/Helpers.h"
#include "WelcomeDialog.h"
#include "ui_WelcomeDialog.h"

WelcomeDialog::WelcomeDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::WelcomeDialog)
{
    ui->setupUi(this);
    ui->logoSvgWidget->load(Config()->getLogoFile());
    ui->versionLabel->setText("<font color='#a4a9b2'>" + tr("Version ") + CUTTER_VERSION_FULL + "</font>");
    ui->themeComboBox->setCurrentIndex(Config()->getTheme());
    ui->themeComboBox->setFixedWidth(200);
    ui->themeComboBox->view()->setFixedWidth(200);

}

WelcomeDialog::~WelcomeDialog()
{
    delete ui;
}

void WelcomeDialog::on_themeComboBox_currentIndexChanged(int index)
{
    Config()->setTheme(index);
    ui->logoSvgWidget->load(Config()->getLogoFile());
}
