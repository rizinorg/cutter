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
    this->main->seek(offset, name, true);
}

void FlagsWidget::on_flagspaceCombo_currentTextChanged(const QString &arg1)
{
    QNOTUSED(arg1);

    refreshFlags();
}

void FlagsWidget::refreshFlagspaces()
{
    int cur_idx = ui->flagspaceCombo->currentIndex();
    if (cur_idx < 0)
        cur_idx = 0;

    ui->flagspaceCombo->clear();
    ui->flagspaceCombo->addItem(tr("(all)"));

    for (auto i : main->core->getAllFlagspaces())
    {
        ui->flagspaceCombo->addItem(i.name, QVariant::fromValue(i));
    }

    if (cur_idx > 0)
        ui->flagspaceCombo->setCurrentIndex(cur_idx);

    refreshFlags();
}

void FlagsWidget::refreshFlags()
{
    QString flagspace;

    QVariant flagspace_data = ui->flagspaceCombo->currentData();
    if(flagspace_data.isValid())
        flagspace = flagspace_data.value<FlagspaceDescription>().name;


    ui->flagsTreeWidget->clear();

    QStringList flags;

    for (auto i : main->core->getAllFlags(flagspace))
    {
        QTreeWidgetItem *item = qhelpers::appendRow(ui->flagsTreeWidget, RSizeString(i.size), RAddressString(i.offset), i.name);
        item->setData(0, Qt::UserRole, QVariant::fromValue(i));

        flags.append(i.name);
    }
    qhelpers::adjustColumns(ui->flagsTreeWidget);

    main->refreshOmniBar(flags);
}

void FlagsWidget::setScrollMode()
{
    qhelpers::setVerticalScrollMode(ui->flagsTreeWidget);
}
