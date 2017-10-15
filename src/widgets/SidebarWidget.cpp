#include "SidebarWidget.h"
#include "ui_SidebarWidget.h"
#include "DisassemblerGraphView.h"

#include "utils/Helpers.h"

#include <QTemporaryFile>
#include <QFontDialog>
#include <QScrollBar>
#include <QClipboard>
#include <QShortcut>
#include <QMenu>
#include <QFont>
#include <QUrl>
#include <QSettings>


SidebarWidget::SidebarWidget(QWidget *parent, Qt::WindowFlags flags) :
        QDockWidget(parent, flags),
        ui(new Ui::SidebarWidget),
        core(CutterCore::getInstance())
{
    ui->setupUi(this);
    this->xrefToTreeWidget_2 = ui->xrefToTreeWidget_2;
    this->xreFromTreeWidget_2 = ui->xreFromTreeWidget_2;

    // Add margin to function name line edit
    ui->fcnNameEdit->setTextMargins(5, 0, 0, 0);

    connect(core, SIGNAL(seekChanged(RVA)), this, SLOT(on_seekChanged(RVA)));
    connect(core, SIGNAL(flagsChanged()), this, SLOT(refresh()));
    connect(core, SIGNAL(commentsChanged()), this, SLOT(refresh()));
    connect(core, SIGNAL(asmOptionsChanged()), this, SLOT(refresh()));

    setScrollMode();
}

SidebarWidget::SidebarWidget(const QString &title, QWidget *parent, Qt::WindowFlags flags)
    : SidebarWidget(parent, flags)
{
    setWindowTitle(title);
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
    if(addr == RVA_INVALID)
        addr = core->getSeekAddr();

    get_refs_data(addr);
    setFcnName(addr);
    fillOffsetInfo(RAddressString(addr));
}

/*
 * Context menu functions
 */

void SidebarWidget::on_offsetToolButton_clicked()
{
    if (ui->offsetToolButton->isChecked())
    {
        ui->offsetTreeWidget->hide();
        ui->offsetToolButton->setArrowType(Qt::RightArrow);
    }
    else
    {
        ui->offsetTreeWidget->show();
        ui->offsetToolButton->setArrowType(Qt::DownArrow);
    }
}

void SidebarWidget::on_xreFromTreeWidget_2_itemDoubleClicked(QTreeWidgetItem *item, int /*column*/)
{
    XrefDescription xref = item->data(0, Qt::UserRole).value<XrefDescription>();
    this->core->seek(xref.to);
}

void SidebarWidget::on_xrefToTreeWidget_2_itemDoubleClicked(QTreeWidgetItem *item, int /*column*/)
{
    XrefDescription xref = item->data(0, Qt::UserRole).value<XrefDescription>();
    this->core->seek(xref.from);
}

void SidebarWidget::on_xrefFromToolButton_2_clicked()
{
    if (ui->xrefFromToolButton_2->isChecked())
    {
        ui->xreFromTreeWidget_2->hide();
        ui->xrefFromToolButton_2->setArrowType(Qt::RightArrow);
    }
    else
    {
        ui->xreFromTreeWidget_2->show();
        ui->xrefFromToolButton_2->setArrowType(Qt::DownArrow);
    }
}

void SidebarWidget::on_xrefToToolButton_2_clicked()
{
    if (ui->xrefToToolButton_2->isChecked())
    {
        ui->xrefToTreeWidget_2->hide();
        ui->xrefToToolButton_2->setArrowType(Qt::RightArrow);
    }
    else
    {
        ui->xrefToTreeWidget_2->show();
        ui->xrefToToolButton_2->setArrowType(Qt::DownArrow);
    }
}

void SidebarWidget::get_refs_data(RVA addr)
{
    // refs = calls q hace esa funcion
    QList<XrefDescription> refs = core->getXRefs(addr, false, false);

    // xrefs = calls a esa funcion
    QList<XrefDescription> xrefs = core->getXRefs(addr, true, false);

    // Data for the disasm side graph
    QList<int> data;
    //qDebug() << "Refs:" << refs.size();
    data << refs.size();
    //qDebug() << "XRefs:" << xrefs.size();
    data << xrefs.size();
    //qDebug() << "CC: " << this->core->fcnCyclomaticComplexity(offset.toLong(&ok, 16));
    //data << this->core->fcnCyclomaticComplexity(offset.toLong(&ok, 16));
    data << this->core->getCycloComplex(addr);
    //qDebug() << "BB: " << this->core->fcnBasicBlockCount(offset.toLong(&ok, 16));
    data << this->core->fcnBasicBlockCount(addr);
    data << this->core->fcnEndBbs(addr);
    //qDebug() << "MEOW: " + this->core->fcnEndBbs(offset);

    // Update disasm side bar
    this->fill_refs(refs, xrefs, data);
}

void SidebarWidget::fill_refs(QList<XrefDescription> refs, QList<XrefDescription> xrefs, QList<int> graph_data)
{
    this->xreFromTreeWidget_2->clear();
    for (int i = 0; i < refs.size(); ++i)
    {
        XrefDescription xref = refs[i];
        QTreeWidgetItem *tempItem = new QTreeWidgetItem();
        tempItem->setText(0, RAddressString(xref.to));
        tempItem->setText(1, core->disassembleSingleInstruction(xref.from));
        tempItem->setData(0, Qt::UserRole, QVariant::fromValue(xref));
        QString tooltip = this->core->cmd("pdi 10 @ " + QString::number(xref.to)).trimmed();
        tempItem->setToolTip(0, tooltip);
        tempItem->setToolTip(1, tooltip);
        this->xreFromTreeWidget_2->insertTopLevelItem(0, tempItem);
    }
    // Adjust columns to content
    int count = this->xreFromTreeWidget_2->columnCount();
    for (int i = 0; i != count; ++i)
    {
        this->xreFromTreeWidget_2->resizeColumnToContents(i);
    }

    this->xrefToTreeWidget_2->clear();
    for (int i = 0; i < xrefs.size(); ++i)
    {
        XrefDescription xref = xrefs[i];

        QTreeWidgetItem *tempItem = new QTreeWidgetItem();
        tempItem->setText(0, RAddressString(xref.from));
        tempItem->setText(1, core->disassembleSingleInstruction(xref.from));
        tempItem->setData(0, Qt::UserRole, QVariant::fromValue(xref));
        QString tooltip = this->core->cmd("pdi 10 @ " + QString::number(xref.from)).trimmed();

        // TODO wtf is this?
        //tempItem->setToolTip(0, this->core->cmd("pdi 10 @ " + tooltip).trimmed());
        //tempItem->setToolTip(1, this->core->cmd("pdi 10 @ " + tooltip).trimmed());

        this->xrefToTreeWidget_2->insertTopLevelItem(0, tempItem);
    }
    // Adjust columns to content
    int count2 = this->xrefToTreeWidget_2->columnCount();
    for (int i = 0; i != count2; ++i)
    {
        this->xrefToTreeWidget_2->resizeColumnToContents(i);
    }
}

void SidebarWidget::fillOffsetInfo(QString off)
{
    ui->offsetTreeWidget->clear();
    QString raw = this->core->getOffsetInfo(off);
    QList<QString> lines = raw.split("\n", QString::SkipEmptyParts);
            foreach (QString line, lines)
        {
            QList<QString> eles = line.split(":", QString::SkipEmptyParts);
            QTreeWidgetItem *tempItem = new QTreeWidgetItem();
            tempItem->setText(0, eles.at(0).toUpper());
            tempItem->setText(1, eles.at(1));
            ui->offsetTreeWidget->insertTopLevelItem(0, tempItem);
        }

    // Adjust column to contents
    int count = ui->offsetTreeWidget->columnCount();
    for (int i = 0; i != count; ++i)
    {
        ui->offsetTreeWidget->resizeColumnToContents(i);
    }

    // Add opcode description
    QStringList description = this->core->cmd("?d. @ " + off).split(": ");
    if (description.length() >= 2)
    {
        ui->opcodeDescText->setPlainText("# " + description[0] + ":\n" + description[1]);
    }
}

void SidebarWidget::setFcnName(RVA addr)
{
    RAnalFunction *fcn;
    QString addr_string;

    fcn = this->core->functionAt(addr);
    if (fcn)
    {
        QString segment = this->core->cmd("S. @ " + QString::number(addr)).split(" ").last();
        addr_string = segment.trimmed() + ":" + fcn->name;
    }
    else
    {
        addr_string = core->cmdFunctionAt(addr);
    }

    ui->fcnNameEdit->setText(addr_string);
}

void SidebarWidget::setScrollMode()
{
    qhelpers::setVerticalScrollMode(ui->xreFromTreeWidget_2);
    qhelpers::setVerticalScrollMode(ui->xrefToTreeWidget_2);
}