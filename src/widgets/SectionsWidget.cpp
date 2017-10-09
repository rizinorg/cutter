#include "widgets/SectionsWidget.h"
#include "widgets/PieView.h"

#include "MainWindow.h"
#include "utils/Helpers.h"

#include <QtWidgets>
#include <QTreeWidget>

SectionsWidget::SectionsWidget(MainWindow *main, QWidget *parent) :
    QSplitter(main),
    main(main)
{
    Q_UNUSED(parent);

    setupViews();
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    //setStyleSheet("QSplitter::handle:horizontal { width: 3px; } QSplitter::handle:vertical { height: 3px; }");
    setStyleSheet("QSplitter::handle { height: 2px; background-color: rgb(255, 255, 255); image: url(:/img/icons/tabs.svg); }");
}

void SectionsWidget::setup()
{
    tree->clear();

    int row = 0;
    for (auto section : CutterCore::getInstance()->getAllSections())
    {
        if (!section.name.contains("."))
            continue;

        fillSections(row++, section);
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

void SectionsWidget::fillSections(int row, const SectionDescription &section)
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
                                          QColor("#95A5A6")     //COBCRETE
                                        };

    QTreeWidgetItem *tempItem = new QTreeWidgetItem();
    tempItem->setText(0, section.name);
    tempItem->setText(1, RSizeString(section.size));
    tempItem->setText(2, RAddressString(section.vaddr));
    tempItem->setText(3, RAddressString(section.vaddr + section.vsize));
    tempItem->setData(0, Qt::DecorationRole, colors[row % colors.size()]);
    this->tree->insertTopLevelItem(0, tempItem);
}
