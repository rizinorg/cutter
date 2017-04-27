#include "flagswidget.h"
#include "ui_flagswidget.h"

#include "mainwindow.h"
#include "helpers.h"

#include <QDockWidget>
#include <QTreeWidget>
#include <QComboBox>


FlagsWidget::FlagsWidget(MainWindow *main, QWidget *parent) :
    DockWidget(parent),
    ui(new Ui::FlagsWidget),
    main(main)
{
    ui->setupUi(this);

    ui->flagsTreeWidget->hideColumn(0);
}

FlagsWidget::~FlagsWidget()
{
    delete ui;
}

void FlagsWidget::setup()
{
    setScrollMode();

    refreshFlagspaces();
}

void FlagsWidget::refresh()
{
    setup();
}

void FlagsWidget::clear()
{
    ui->flagsTreeWidget->clear();
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

    refreshFlags();
}

void FlagsWidget::refreshFlagspaces()
{
    int cur_idx = ui->flagspaceCombo->currentIndex();
    if (cur_idx < 0)cur_idx = 0;
    ui->flagspaceCombo->clear();
    ui->flagspaceCombo->addItem("(all)");
    for (auto i : main->core->getList("flagspaces"))
    {
        ui->flagspaceCombo->addItem(i);
    }
    if (cur_idx > 0)
        ui->flagspaceCombo->setCurrentIndex(cur_idx);

    refreshFlags();
}

void FlagsWidget::refreshFlags()
{
    QString flagspace = ui->flagspaceCombo->currentText();
    if (flagspace == "(all)")
        flagspace = "";

    ui->flagsTreeWidget->clear();

    QStringList flags;

    for (auto i : main->core->getList("flags", flagspace))
    {
        QStringList a = i.split(",");
        if (a.length() > 3)
        {
            qhelpers::appendRow(ui->flagsTreeWidget, a[1], a[2], a[0], a[3]);
            //this->omnibar->fillFlags(a[0]);
        }
        else if (a.length() > 2)
        {
            qhelpers::appendRow(ui->flagsTreeWidget, a[1], a[2], a[0], "");
            //this->omnibar->fillFlags(a[0]);
        }

        flags.append(a[0]);
    }
    qhelpers::adjustColumns(ui->flagsTreeWidget);

    main->refreshOmniBar(flags);
}

void FlagsWidget::setScrollMode()
{
    qhelpers::setVerticalScrollMode(ui->flagsTreeWidget);
}
