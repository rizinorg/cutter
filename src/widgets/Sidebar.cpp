#include "Sidebar.h"
#include "ui_Sidebar.h"

#include "MainWindow.h"

#include <QSettings>


SideBar::SideBar(MainWindow *main) :
    QWidget(main),
    ui(new Ui::SideBar),
    // Radare core found in:
    main(main)
{
    ui->setupUi(this);

    QSettings settings;
    if (settings.value("responsive").toBool()) {
        ui->respButton->setChecked(true);
    } else {
        ui->respButton->setChecked(false);
    }
}

SideBar::~SideBar() {}

void SideBar::on_tabsButton_clicked()
{
    this->main->on_actionTabs_triggered();
}

void SideBar::on_lockButton_clicked()
{
    if (ui->lockButton->isChecked()) {
        ui->lockButton->setIcon(QIcon(":/img/icons/unlock_white.svg"));
        this->main->lockUnlock_Docks(1);
    } else {
        ui->lockButton->setIcon(QIcon(":/img/icons/lock_white.svg"));
        this->main->lockUnlock_Docks(0);
    }
}

void SideBar::on_calcInput_textChanged(const QString &arg1)
{
    ui->calcOutput->setText(QString::number(Core()->math(arg1)));
}

void SideBar::on_asm2hex_clicked()
{
    ui->hexInput->setPlainText(Core()->assemble(ui->asmInput->toPlainText()));
}

void SideBar::on_hex2asm_clicked()
{
    ui->asmInput->setPlainText(Core()->disassemble(ui->hexInput->toPlainText()));
}

void SideBar::on_respButton_toggled(bool checked)
{
    this->main->toggleResponsive(checked);
}

void SideBar::on_refreshButton_clicked()
{
    this->main->refreshAll();
}
