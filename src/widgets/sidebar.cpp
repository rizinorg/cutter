#include "sidebar.h"
#include "ui_sidebar.h"

#include <QSettings>

#include "mainwindow.h"

SideBar::SideBar(MainWindow *main, QWidget *parent) :
    QWidget(parent),
    ui(new Ui::SideBar)
{
    ui->setupUi(this);

    // Radare core found in:
    this->main = main;

    QSettings settings("iaito", "iaito");
    if (settings.value("responsive").toBool()) {
        ui->respButton->setChecked(true);
    } else {
        ui->respButton->setChecked(false);
    }
}

SideBar::~SideBar()
{
    delete ui;
}

void SideBar::on_tabsButton_clicked()
{
    this->main->on_actionTabs_triggered();
}

void SideBar::on_consoleButton_clicked()
{
    this->main->on_actionhide_bottomPannel_triggered();
    if (ui->consoleButton->isChecked()) {
        ui->consoleButton->setIcon(QIcon(":/new/prefix1/img/icons/up_white.png"));
    } else {
        ui->consoleButton->setIcon(QIcon(":/new/prefix1/img/icons/down_white.png"));
    }
}

void SideBar::on_webServerButton_clicked()
{
    static WebServerThread thread;
    if (ui->webServerButton->isChecked()) {
        // Start web server
        thread.core = this->main->core;
        thread.start();
        QThread::sleep (1);
        if (this->main->core->core->http_up==R_FALSE) {
            eprintf ("FAILED TO LAUNCH\n");
        }
        // Open web interface on default browser
        //QString link = "http://localhost:9090/";
        //QDesktopServices::openUrl(QUrl(link));
    } else {
        this->main->core->core->http_up= R_FALSE;
        // call something to kill the webserver!!
        thread.exit(0);
        // Stop web server
    }
}

void SideBar::on_lockButton_clicked()
{
    if(ui->lockButton->isChecked()) {
        ui->lockButton->setIcon( QIcon(":/new/prefix1/img/icons/unlock_white.png") );
        this->main->lockUnlock_Docks(1);
    } else {
        ui->lockButton->setIcon( QIcon(":/new/prefix1/img/icons/lock_white.png") );
        this->main->lockUnlock_Docks(0);
    }
}

void SideBar::themesButtonToggle() {
    ui->themesButton->click();
}

void SideBar::on_themesButton_clicked()
{
    if (ui->themesButton->isChecked() ) {
        // Dark theme
        this->main->dark();
    } else {
        // Clear theme
        this->main->def_theme();
    }
}

void SideBar::on_calcInput_textChanged(const QString &arg1)
{
    ui->calcOutput->setText(QString::number(this->main->core->math (arg1)));
}

void SideBar::on_asm2hex_clicked()
{
    ui->hexInput->setPlainText(main->core->assemble(ui->asmInput->toPlainText()));
}

void SideBar::on_hex2asm_clicked()
{
    ui->asmInput->setPlainText(main->core->assemble(ui->hexInput->toPlainText()));
}

void SideBar::on_respButton_toggled(bool checked)
{
    this->main->toggleResponsive(checked);
}
