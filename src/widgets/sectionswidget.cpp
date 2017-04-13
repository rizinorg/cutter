#include "widgets/sectionswidget.h"
#include "widgets/pieview.h"

#include "mainwindow.h"
#include "helpers.h"

#include <QtWidgets>
#include <QTreeWidget>

SectionsWidget::SectionsWidget(MainWindow *main, QWidget *parent) :
    QSplitter(main),
    main(main)
{
    QNOTUSED(parent);

    setupViews();
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    //setStyleSheet("QSplitter::handle:horizontal { width: 3px; } QSplitter::handle:vertical { height: 3px; }");
    setStyleSheet("QSplitter::handle { height: 2px; background-color: rgb(255, 255, 255); image: url(:/new/prefix1/img/icons/tabs.png); }");
}

void SectionsWidget::setup()
{
    tree->clear();

    int row = 0;
    for (auto i : main->core->getList("bin", "sections"))
    {
        QStringList a = i.split(",");
        if (a.length() > 4)
        {
            // Fix to work with ARM bins
            //if (a[4].startsWith(".")) {
            if (a[4].contains("."))
            {
                QString addr = a[1];
                QString addr_end = "0x0" + main->core->itoa(main->core->math(addr + "+" + a[2]));
                QString size = QString::number(main->core->math(a[2]));
                QString name = a[4];

                fillSections(row++, name, size, addr, addr_end);
            }
        }
    }
    //adjustColumns(sectionsWidget->tree);
    //this->sectionsDock->sectionsWidget->adjustColumns();
    qhelpers::adjustColumns(tree);
}

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

void SectionsWidget::fillSections(int row, const QString &str, const QString &str2,
                                  const QString &str3, const QString &str4)
{
    // TODO: create unique colors, e. g. use HSV color space and rotate in H for 360/size
    static const QList<QColor> colors = { QColor("#1ABC9C"),    //TURQUOISE
                                          QColor("#2ECC71"),    //EMERALD
                                          QColor("#3498DB"),    //PETER RIVER
                                          QColor("#9B59B6"),    //AMETHYST
                                          QColor("#34495E"),    //WET ASPHALT
                                          QColor("#F1C40F"),    //SUN FLOWER
                                          QColor("#E67E22"),    //CARROT
                                          QColor("#E74C3C"),    //ALIZARIN
                                          QColor("#ECF0F1"),    //CLOUDS
                                          QColor("#BDC3C7"),    //SILVER
                                          QColor("#95A5A6")};   //COBCRETE

    QTreeWidgetItem *tempItem = new QTreeWidgetItem();
    tempItem->setText(0, str);
    tempItem->setText(1, str2);
    tempItem->setText(2, str3);
    tempItem->setText(3, str4);
    tempItem->setData(0, Qt::DecorationRole, colors[row % colors.size()]);
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
