#include <QMenu>
#include <QSplitter>
#include <QTreeView>
#include <QResizeEvent>

#include "SectionsWidget.h"
#include "ui_SectionsWidget.h"
#include "PieView.h"

#include "MainWindow.h"
#include "utils/Helpers.h"

SectionsModel::SectionsModel(QList<SectionDescription> *sections, QObject *parent)
    : QAbstractListModel(parent),
      sections(sections)
{
}

int SectionsModel::rowCount(const QModelIndex &) const
{
    return sections->count();
}

int SectionsModel::columnCount(const QModelIndex &) const
{
    return SectionsModel::ColumnCount;
}

QVariant SectionsModel::data(const QModelIndex &index, int role) const
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

    if (index.row() >= sections->count())
        return QVariant();

    const SectionDescription &section = sections->at(index.row());

    switch (role) {
    case Qt::DisplayRole:
        switch (index.column()) {
        case SectionsModel::NameColumn:
            return section.name;
        case SectionsModel::SizeColumn:
            return section.size;
        case SectionsModel::AddressColumn:
            return RAddressString(section.vaddr);
        case SectionsModel::EndAddressColumn:
            return RAddressString(section.vaddr + section.size);
        case SectionsModel::EntropyColumn:
            return section.entropy;
        default:
            return QVariant();
        }
    case Qt::DecorationRole:
        if (index.column() == 0)
            return colors[index.row() % colors.size()];
        return QVariant();
    case SectionsModel::SectionDescriptionRole:
        return QVariant::fromValue(section);
    default:
        return QVariant();
    }
}

QVariant SectionsModel::headerData(int section, Qt::Orientation, int role) const
{
    switch (role) {
    case Qt::DisplayRole:
        switch (section) {
        case SectionsModel::NameColumn:
            return tr("Name");
        case SectionsModel::SizeColumn:
            return tr("Size");
        case SectionsModel::AddressColumn:
            return tr("Address");
        case SectionsModel::EndAddressColumn:
            return tr("EndAddress");
        case SectionsModel::EntropyColumn:
            return tr("Entropy");
        default:
            return QVariant();
        }
    default:
        return QVariant();
    }
}

void SectionsModel::beginReloadSections()
{
    beginResetModel();
}

void SectionsModel::endReloadSections()
{
    endResetModel();
    // Update PieChart
    emit dataChanged(QModelIndex(), QModelIndex());
}

SectionsProxyModel::SectionsProxyModel(SectionsModel *sourceModel, QObject *parent)
    : QSortFilterProxyModel(parent)
{
    setSourceModel(sourceModel);
    setFilterCaseSensitivity(Qt::CaseInsensitive);
    setSortCaseSensitivity(Qt::CaseInsensitive);
    connect(sourceModel, SIGNAL(dataChanged(QModelIndex,QModelIndex,QVector<int>)),
            this, SLOT(onSourceModelDataChanged(QModelIndex,QModelIndex,QVector<int>)));
}

void SectionsProxyModel::onSourceModelDataChanged(const QModelIndex &topLeft,
                                                  const QModelIndex &bottomRight,
                                                  const QVector<int> &roles)
{
    // Pass the signal further to update PieChart
    emit dataChanged(topLeft, bottomRight, roles);
}

bool SectionsProxyModel::lessThan(const QModelIndex &left, const QModelIndex &right) const
{
    auto leftSection = left.data(SectionsModel::SectionDescriptionRole).value<SectionDescription>();
    auto rightSection = right.data(SectionsModel::SectionDescriptionRole).value<SectionDescription>();

    switch (left.column()) {
    case SectionsModel::NameColumn:
        return leftSection.name < rightSection.name;
    case SectionsModel::SizeColumn:
        return leftSection.size < rightSection.size;
    case SectionsModel::AddressColumn:
    case SectionsModel::EndAddressColumn:
        return leftSection.vaddr < rightSection.vaddr;
    case SectionsModel::EntropyColumn:
        return leftSection.entropy < rightSection.entropy;

    default:
        break;
    }

    return false;
}

SectionsWidget::SectionsWidget(MainWindow *main, QAction *action) :
    CutterDockWidget(main, action),
    ui(new Ui::SectionsWidget),
    main(main)
{
    ui->setupUi(this);

    sectionsModel = new SectionsModel(&sections, this);
    sectionsProxyModel = new SectionsProxyModel(sectionsModel, this);

    setupViews();

    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);

    setContextMenuPolicy(Qt::CustomContextMenu);
    connect(this, SIGNAL(customContextMenuRequested(const QPoint &)),
            this, SLOT(showSectionsContextMenu(const QPoint &)));

    connect(Core(), SIGNAL(refreshAll()), this, SLOT(refreshSections()));
}

SectionsWidget::~SectionsWidget() {}

void SectionsWidget::resizeEvent(QResizeEvent *event)
{
    if (main->responsive && isVisible()) {
        if (event->size().width() >= event->size().height()) {
            on_actionHorizontal_triggered();
        } else {
            on_actionVertical_triggered();
        }
    }
    QWidget::resizeEvent(event);
}

void SectionsWidget::showSectionsContextMenu(const QPoint &pt)
{
    // Set functions popup menu
    QMenu *menu = new QMenu(this);
    menu->clear();
    menu->addAction(ui->actionHorizontal);
    menu->addAction(ui->actionVertical);

    if (splitter->orientation() == 1) {
        ui->actionHorizontal->setChecked(true);
        ui->actionVertical->setChecked(false);
    } else {
        ui->actionVertical->setChecked(true);
        ui->actionHorizontal->setChecked(false);
    }

    menu->exec(mapToGlobal(pt));
    delete menu;
}

void SectionsWidget::refreshSections()
{
    sectionsModel->beginReloadSections();
    sections = Core()->getAllSections();
    sectionsModel->endReloadSections();

    qhelpers::adjustColumns(sectionsTable, SectionsModel::ColumnCount, 0);
}

void SectionsWidget::setupViews()
{
    splitter = new QSplitter;
    sectionsTable = new QTreeView;
    sectionsPieChart = new PieView;

    splitter->addWidget(sectionsTable);
    splitter->addWidget(sectionsPieChart);
    //splitter->setStretchFactor(0, 4);

    sectionsTable->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    sectionsTable->setIndentation(10);
    sectionsTable->setFrameShape(QFrame::NoFrame);
    sectionsTable->setSortingEnabled(true);
    sectionsTable->sortByColumn(SectionsModel::NameColumn, Qt::AscendingOrder);
    connect(sectionsTable, SIGNAL(doubleClicked(const QModelIndex &)),
            this, SLOT(onSectionsDoubleClicked(const QModelIndex &)));

    sectionsPieChart->setFrameShape(QFrame::NoFrame);
    sectionsPieChart->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    sectionsTable->setModel(sectionsProxyModel);
    sectionsPieChart->setModel(sectionsProxyModel);

    QItemSelectionModel *selectionModel = new QItemSelectionModel(sectionsProxyModel);
    sectionsTable->setSelectionModel(selectionModel);
    sectionsPieChart->setSelectionModel(selectionModel);

    setWidget(splitter);
}

void SectionsWidget::onSectionsDoubleClicked(const QModelIndex &index)
{
    if (!index.isValid())
        return;

    auto section = index.data(SectionsModel::SectionDescriptionRole).value<SectionDescription>();
    Core()->seek(section.vaddr);
}

void SectionsWidget::on_actionVertical_triggered()
{
    splitter->setOrientation(Qt::Vertical);
}

void SectionsWidget::on_actionHorizontal_triggered()
{
    splitter->setOrientation(Qt::Horizontal);
}
