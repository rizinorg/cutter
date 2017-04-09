#include "flagswidget.h"
#include "ui_flagswidget.h"

#include "mainwindow.h"

FlagsWidget::FlagsWidget(MainWindow *main, QWidget *parent) :
    QDockWidget(parent),
    ui(new Ui::FlagsWidget)
{
    ui->setupUi(this);

    // Radare core found in:
    this->main = main;

    this->flagsTreeWidget = ui->flagsTreeWidget;
    this->flagspaceCombo = ui->flagspaceCombo;
}

FlagsWidget::~FlagsWidget()
{
    delete ui;
}

void FlagsWidget::on_flagsTreeWidget_itemDoubleClicked(QTreeWidgetItem *item, int column)
{
    QNOTUSED(column);

    QString offset = item->text(2);
    QString name = item->text(3);
    this->main->seek(offset, name);
}

void FlagsWidget::on_flagspaceCombo_currentTextChanged(const QString &arg1)
{
    QNOTUSED(arg1);

    this->main->refreshFlags();
}
