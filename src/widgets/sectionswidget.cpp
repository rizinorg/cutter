#include <QtWidgets>
#include <QSplitter>

#include "widgets/pieview.h"
#include "widgets/sectionswidget.h"

#include "mainwindow.h"

SectionsWidget::SectionsWidget(MainWindow *main, QWidget *parent) :
    QSplitter(main)
{
    QNOTUSED(parent);

    this->main = main;
    //setupModel();
    setupViews();
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    //setStyleSheet("QSplitter::handle:horizontal { width: 3px; } QSplitter::handle:vertical { height: 3px; }");
    setStyleSheet("QSplitter::handle { height: 2px; background-color: rgb(255, 255, 255); image: url(:/new/prefix1/img/icons/tabs.png); }");
}

/*
void SectionsWidget::setupModel()
{
    model = new QStandardItemModel(0, 4, this);
    model->setHeaderData(0, Qt::Horizontal, "Name");
    model->setHeaderData(1, Qt::Horizontal, "Size");
    model->setHeaderData(2, Qt::Horizontal, "Address");
    model->setHeaderData(3, Qt::Horizontal, "End Address");
}
*/
void SectionsWidget::setupViews()
{
    // Table view
    this->tree = new QTreeWidget;
    this->tree->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    //this->tree->setFont(QFont("Lucida Grande UI", 12));
    //this->tree->setFont(QFont("Courier New", 11));
    this->tree->setIndentation(10);
    //this->tree->setStyleSheet("QTreeWidget::item { padding-top: 1px; padding-bottom: 1px; padding-left:10px; border-left:10px;} QTreeWidget::item:selected { background: gray; color: white; } QTreeWidget::item:hover { background: rgb(242, 246, 248); color: black; }");

    // Setup TreeWidget
    this->tree->setColumnCount(4);
    QList<QString> headers;
    headers << "Name" << "Size" << "Address" << "End Address";
    this->tree->setHeaderLabels(headers);

    this->tree->setFrameShape(QFrame::NoFrame);
    this->tree->setSortingEnabled(true);

    pieChart = new PieView;
    pieChart->setFrameShape(QFrame::NoFrame);
    pieChart->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    this->addWidget(this->tree);
    this->addWidget(pieChart);
    this->setStretchFactor(0, 4);

    //this->tree->setModel(model);
    pieChart->setModel(this->tree->model());

    QItemSelectionModel *selectionModel = new QItemSelectionModel(this->tree->model());
    this->tree->setSelectionModel(selectionModel);
    pieChart->setSelectionModel(selectionModel);
}

void SectionsWidget::fillSections(int row, const QString &str, const QString &str2 = NULL,
                                  const QString &str3 = NULL, const QString &str4 = NULL)
{
    QList<QString> colors;
    //colors << "#F7464A" << "#46BFBD" << "#FDB45C" << "#949FB1" << "#4D5360" << "#D97041" <<"#C7604C" << "#21323D" << "#9D9B7F" << "#7D4F6D" << "#584A5E";
    colors << "#1ABC9C";    //TURQUOISE
    colors << "#2ECC71";    //EMERALD
    colors << "#3498DB";    //PETER RIVER
    colors << "#9B59B6";    //AMETHYST
    colors << "#34495E";    //WET ASPHALT
    colors << "#F1C40F";    //SUN FLOWER
    colors << "#E67E22";    //CARROT
    colors << "#E74C3C";    //ALIZARIN
    colors << "#ECF0F1";    //CLOUDS
    colors << "#BDC3C7";    //SILVER
    colors << "#95A5A6";    //COBCRETE

    QTreeWidgetItem *tempItem = new QTreeWidgetItem();
    tempItem->setText(0, str);
    tempItem->setText(1, str2);
    tempItem->setText(2, str3);
    tempItem->setText(3, str4);
    tempItem->setData(0, Qt::DecorationRole, QColor(colors[row]));
    this->tree->insertTopLevelItem(0, tempItem);
}

void SectionsWidget::adjustColumns()
{
    int count = 4;
    for (int i = 0; i != count; ++i)
    {
        this->tree->resizeColumnToContents(i);
    }
}
