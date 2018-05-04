#include "SidebarWidget.h"
#include "ui_SidebarWidget.h"
#include "DisassemblerGraphView.h"

#include "utils/Helpers.h"
#include "utils/TempConfig.h"

#include <QTemporaryFile>
#include <QFontDialog>
#include <QScrollBar>
#include <QShortcut>
#include <QMenu>
#include <QFont>
#include <QUrl>
#include <QSettings>
#include <QJsonObject>
#include <QJsonArray>

SidebarWidget::SidebarWidget(MainWindow *main, QAction *action) :
    CutterDockWidget(main, action),
    ui(new Ui::SidebarWidget)
{
    ui->setupUi(this);

    // Add margin to function name line edit
    ui->fcnNameEdit->setTextMargins(5, 0, 0, 0);

    setScrollMode();

    connect(Core(), SIGNAL(seekChanged(RVA)), this, SLOT(on_seekChanged(RVA)));
    connect(Core(), SIGNAL(flagsChanged()), this, SLOT(refresh()));
    connect(Core(), SIGNAL(commentsChanged()), this, SLOT(refresh()));
    connect(Core(), SIGNAL(asmOptionsChanged()), this, SLOT(refresh()));

    connect(Core(), SIGNAL(refreshAll()), this, SLOT(refresh()));
}

SidebarWidget::~SidebarWidget()
{
}

void SidebarWidget::on_seekChanged(RVA addr)
{
    refresh(addr);
}

void SidebarWidget::refresh(RVA addr)
{
    if (addr == RVA_INVALID)
        addr = Core()->getOffset();

    updateRefs(addr);
    setFcnName(addr);
    fillOffsetInfo(RAddressString(addr));
    fillRegistersInfo();
}

void SidebarWidget::on_xrefFromTreeWidget_itemDoubleClicked(QTreeWidgetItem *item, int column)
{
    if (column < 0)
        return;

    XrefDescription xref = item->data(0, Qt::UserRole).value<XrefDescription>();
    Core()->seek(xref.to);
}

void SidebarWidget::on_xrefToTreeWidget_itemDoubleClicked(QTreeWidgetItem *item, int column)
{
    if (column < 0)
        return;

    XrefDescription xref = item->data(0, Qt::UserRole).value<XrefDescription>();
    Core()->seek(xref.from);
}

void SidebarWidget::on_offsetToolButton_clicked()
{
    if (ui->offsetToolButton->isChecked()) {
        ui->offsetTreeWidget->hide();
        ui->offsetToolButton->setArrowType(Qt::RightArrow);
    } else {
        ui->offsetTreeWidget->show();
        ui->offsetToolButton->setArrowType(Qt::DownArrow);
    }
}

void SidebarWidget::on_opcodeDescToolButton_clicked()
{
    if (ui->opcodeDescToolButton->isChecked()) {
        ui->opcodeDescText->hide();
        ui->opcodeDescToolButton->setArrowType(Qt::RightArrow);
    } else {
        ui->opcodeDescText->show();
        ui->opcodeDescToolButton->setArrowType(Qt::DownArrow);
    }
}

void SidebarWidget::on_xrefFromToolButton_clicked()
{
    if (ui->xrefFromToolButton->isChecked()) {
        ui->xrefFromTreeWidget->hide();
        ui->xrefFromToolButton->setArrowType(Qt::RightArrow);
    } else {
        ui->xrefFromTreeWidget->show();
        ui->xrefFromToolButton->setArrowType(Qt::DownArrow);
    }
}

void SidebarWidget::on_xrefToToolButton_clicked()
{
    if (ui->xrefToToolButton->isChecked()) {
        ui->xrefToTreeWidget->hide();
        ui->xrefToToolButton->setArrowType(Qt::RightArrow);
    } else {
        ui->xrefToTreeWidget->show();
        ui->xrefToToolButton->setArrowType(Qt::DownArrow);
    }
}

void SidebarWidget::on_regInfoToolButton_clicked()
{
    if (ui->regInfoToolButton->isChecked()) {
        ui->regInfoTreeWidget->hide();
        ui->regInfoToolButton->setArrowType(Qt::RightArrow);
    } else {
        ui->regInfoTreeWidget->show();
        ui->regInfoToolButton->setArrowType(Qt::DownArrow);
    }
}

void SidebarWidget::updateRefs(RVA addr)
{
    // refs = calls q hace esa funcion
    QList<XrefDescription> refs = Core()->getXRefs(addr, false, false);

    // xrefs = calls a esa funcion
    QList<XrefDescription> xrefs = Core()->getXRefs(addr, true, false);

    // Update disasm side bar
    this->fillRefs(refs, xrefs);
}

void SidebarWidget::fillRefs(QList<XrefDescription> refs, QList<XrefDescription> xrefs)
{
    TempConfig tempConfig;
    tempConfig.set("scr.html", false)
    .set("scr.color", COLOR_MODE_DISABLED);

    ui->xrefFromTreeWidget->clear();
    for (int i = 0; i < refs.size(); ++i) {
        XrefDescription xref = refs[i];
        QTreeWidgetItem *tempItem = new QTreeWidgetItem();
        tempItem->setText(0, xref.to_str);
        tempItem->setText(1, Core()->disassembleSingleInstruction(xref.to));
        tempItem->setData(0, Qt::UserRole, QVariant::fromValue(xref));
        QString tooltip = Core()->cmd("pdi 10 @ " + QString::number(xref.to)).trimmed();
        tempItem->setToolTip(0, tooltip);
        tempItem->setToolTip(1, tooltip);
        ui->xrefFromTreeWidget->insertTopLevelItem(0, tempItem);
    }
    // Adjust columns to content
    qhelpers::adjustColumns(ui->xrefFromTreeWidget, 0);

    ui->xrefToTreeWidget->clear();
    for (int i = 0; i < xrefs.size(); ++i) {
        XrefDescription xref = xrefs[i];

        QTreeWidgetItem *tempItem = new QTreeWidgetItem();
        tempItem->setText(0, xref.from_str);
        tempItem->setText(1, Core()->disassembleSingleInstruction(xref.from));
        tempItem->setData(0, Qt::UserRole, QVariant::fromValue(xref));
        QString tooltip = Core()->cmd("pdi 10 @ " + QString::number(xref.from)).trimmed();

        // TODO wtf is this?
        //tempItem->setToolTip(0, this->core->cmd("pdi 10 @ " + tooltip).trimmed());
        //tempItem->setToolTip(1, this->core->cmd("pdi 10 @ " + tooltip).trimmed());

        ui->xrefToTreeWidget->insertTopLevelItem(0, tempItem);
    }
    // Adjust columns to content
    qhelpers::adjustColumns(ui->xrefToTreeWidget, 0);
}

void SidebarWidget::fillOffsetInfo(QString off)
{
    TempConfig tempConfig;
    tempConfig.set("scr.html", false)
    .set("scr.color", COLOR_MODE_DISABLED);

    ui->offsetTreeWidget->clear();
    QString raw = Core()->getOffsetInfo(off);
    QList<QString> lines = raw.split("\n", QString::SkipEmptyParts);
    for (QString line : lines) {
        QList<QString> eles = line.split(":", QString::SkipEmptyParts);
        QTreeWidgetItem *tempItem = new QTreeWidgetItem();
        tempItem->setText(0, eles.at(0).toUpper());
        tempItem->setText(1, eles.at(1));
        ui->offsetTreeWidget->insertTopLevelItem(0, tempItem);
    }

    // Adjust column to contents
    qhelpers::adjustColumns(ui->offsetTreeWidget, 0);

    // Add opcode description
    QStringList description = Core()->cmd("?d. @ " + off).split(": ");
    if (description.length() >= 2) {
        ui->opcodeDescText->setPlainText("# " + description[0] + ":\n" + description[1]);
    }
}

void SidebarWidget::setFcnName(RVA addr)
{
    RAnalFunction *fcn;
    QString addr_string;

    fcn = Core()->functionAt(addr);
    if (fcn) {
        QString segment = Core()->cmd("S. @ " + QString::number(addr)).split(" ").last();
        addr_string = segment.trimmed() + ":" + fcn->name;
    } else {
        addr_string = Core()->cmdFunctionAt(addr);
    }

    ui->fcnNameEdit->setText(addr_string);
}

void SidebarWidget::setScrollMode()
{
    qhelpers::setVerticalScrollMode(ui->xrefFromTreeWidget);
    qhelpers::setVerticalScrollMode(ui->xrefToTreeWidget);
}

void SidebarWidget::fillRegistersInfo()
{
    TempConfig tempConfig;
    tempConfig.set("scr.html", false)
    .set("scr.color", COLOR_MODE_DISABLED);

    ui->regInfoTreeWidget->clear();

    QJsonObject jsonRoot = Core()->getRegistersInfo().object();
    for (QString key : jsonRoot.keys()) {
        QTreeWidgetItem *tempItem = new QTreeWidgetItem();
        QString tempString;
        tempItem->setText(0, key.toUpper());
        for (QJsonValue value : jsonRoot[key].toArray()) {
            tempString.append(value.toString() + " ");
        }
        tempItem->setText(1, tempString);
        ui->regInfoTreeWidget->addTopLevelItem(tempItem);
    }

    // Adjust columns to content
    qhelpers::adjustColumns(ui->regInfoTreeWidget, 0);
}
