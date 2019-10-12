#include "SearchWidget.h"
#include "ui_SearchWidget.h"
#include "core/MainWindow.h"
#include "common/Helpers.h"

#include <QDockWidget>
#include <QTreeWidget>
#include <QComboBox>
#include <QShortcut>

namespace {

static const int kMaxTooltipWidth = 500;
static const int kMaxTooltipDisasmPreviewLines = 10;
static const int kMaxTooltipHexdumpBytes = 64;

}

static const QMap<QString, QString> kSearchBoundariesValues {
    {"io.maps", "All maps"},
    {"io.map", "Current map"},
    {"raw", "Raw"},
    {"dbg.maps", "All memory maps"},
    {"dbg.map", "Memory map"},
    {"block", "Current block"},
    {"bin.section", "Current mapped section"},
    {"bin.sections", "All mapped sections"},
    {"dbg.stack", "Stack"},
    {"dbg.heap", "Heap"}
   };

SearchModel::SearchModel(QList<SearchDescription> *search, QObject *parent)
    : AddressableItemModel<QAbstractListModel>(parent),
      search(search)
{
}

int SearchModel::rowCount(const QModelIndex &) const
{
    return search->count();
}

int SearchModel::columnCount(const QModelIndex &) const
{
    return Columns::COUNT;
}

QVariant SearchModel::data(const QModelIndex &index, int role) const
{
    if (index.row() >= search->count())
        return QVariant();

    const SearchDescription &exp = search->at(index.row());

    switch (role) {
    case Qt::DisplayRole:
        switch (index.column()) {
        case OFFSET:
            return RAddressString(exp.offset);
        case SIZE:
            return RSizeString(exp.size);
        case CODE:
            return exp.code;
        case DATA:
            return exp.data;
        default:
            return QVariant();
        }
    case Qt::ToolTipRole: {

        QString previewContent = QString();
        // if result is CODE, show disassembly
        if (!exp.code.isEmpty()) {
            previewContent = Core()->getDisassemblyPreview(exp.offset, kMaxTooltipDisasmPreviewLines)
                                    .join("<br>");
        // if result is DATA or Disassembly is N/A                                    
        } else if (!exp.data.isEmpty() || previewContent.isEmpty()) {
            previewContent = Core()->getHexdumpPreview(exp.offset, kMaxTooltipHexdumpBytes);
        }

        const QFont &fnt = Config()->getBaseFont();
        QFontMetrics fm{ fnt };

        QString toolTipContent = QString("<html><div style=\"font-family: %1; font-size: %2pt; white-space: nowrap;\">")
                .arg(fnt.family())
                .arg(qMax(6, fnt.pointSize() - 1)); // slightly decrease font size, to keep more text in the same box
        
        toolTipContent += tr("<div style=\"margin-bottom: 10px;\"><strong>Preview</strong>:<br>%1</div>")
                .arg(previewContent);

        toolTipContent += "</div></html>";
        return toolTipContent;
    }
    case SearchDescriptionRole:
        return QVariant::fromValue(exp);
    default:
        return QVariant();
    }
}

QVariant SearchModel::headerData(int section, Qt::Orientation, int role) const
{
    switch (role) {
    case Qt::DisplayRole:
        switch (section) {
        case SIZE:
            return tr("Size");
        case OFFSET:
            return tr("Offset");
        case CODE:
            return tr("Code");
        case DATA:
            return tr("Data");
        default:
            return QVariant();
        }
    default:
        return QVariant();
    }
}

RVA SearchModel::address(const QModelIndex &index) const
{
    const SearchDescription &exp = search->at(index.row());
    return exp.offset;
}


SearchSortFilterProxyModel::SearchSortFilterProxyModel(SearchModel *source_model, QObject *parent)
    : AddressableFilterProxyModel(source_model, parent)
{
}

bool SearchSortFilterProxyModel::filterAcceptsRow(int row, const QModelIndex &parent) const
{
    QModelIndex index = sourceModel()->index(row, 0, parent);
    SearchDescription search = index.data(
                                   SearchModel::SearchDescriptionRole).value<SearchDescription>();
    return search.code.contains(filterRegExp());
}

bool SearchSortFilterProxyModel::lessThan(const QModelIndex &left, const QModelIndex &right) const
{
    SearchDescription left_search = left.data(
                                        SearchModel::SearchDescriptionRole).value<SearchDescription>();
    SearchDescription right_search = right.data(
                                         SearchModel::SearchDescriptionRole).value<SearchDescription>();

    switch (left.column()) {
    case SearchModel::SIZE:
        return left_search.size < right_search.size;
    case SearchModel::OFFSET:
        return left_search.offset < right_search.offset;
    case SearchModel::CODE:
        return left_search.code < right_search.code;
    case SearchModel::DATA:
        return left_search.data < right_search.data;
    default:
        break;
    }

    return left_search.offset < right_search.offset;
}


SearchWidget::SearchWidget(MainWindow *main, QAction *action) :
    CutterDockWidget(main, action),
    ui(new Ui::SearchWidget)
{
    ui->setupUi(this);
    setStyleSheet(QString("QToolTip { max-width: %1px; opacity: 230; }").arg(kMaxTooltipWidth));

    ui->searchInCombo->blockSignals(true);
    QMap<QString, QString>::const_iterator mapIter;
    for (mapIter = kSearchBoundariesValues.cbegin(); mapIter != kSearchBoundariesValues.cend(); ++mapIter)
        ui->searchInCombo->addItem(mapIter.value(), mapIter.key());
    ui->searchInCombo->blockSignals(false);

    search_model = new SearchModel(&search, this);
    search_proxy_model = new SearchSortFilterProxyModel(search_model, this);
    ui->searchTreeView->setModel(search_proxy_model);
    ui->searchTreeView->setMainWindow(main);
    ui->searchTreeView->sortByColumn(SearchModel::OFFSET, Qt::AscendingOrder);

    setScrollMode();

    connect(Core(), SIGNAL(refreshAll()), this, SLOT(refreshSearchspaces()));

    QShortcut *enter_press = new QShortcut(QKeySequence(Qt::Key_Return), this);
    connect(enter_press, &QShortcut::activated, this, [this]() {
        refreshSearch();
    });
    enter_press->setContext(Qt::WidgetWithChildrenShortcut);

    connect(ui->searchButton, &QAbstractButton::clicked, this, [this]() {
        refreshSearch();
    });

    connect(ui->searchspaceCombo, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
            this, [this](int index) { updatePlaceholderText(index);});

    QString currentSearchBoundary = Core()->getConfig("search.in");
    ui->searchInCombo->setCurrentIndex(ui->searchInCombo->findData(currentSearchBoundary));
}

SearchWidget::~SearchWidget() {}

void SearchWidget::searchChanged()
{
    refreshSearchspaces();
}

void SearchWidget::refreshSearchspaces()
{
    int cur_idx = ui->searchspaceCombo->currentIndex();
    if (cur_idx < 0)
        cur_idx = 0;

    ui->searchspaceCombo->clear();
    ui->searchspaceCombo->addItem(tr("asm code"),   QVariant("/acj"));
    ui->searchspaceCombo->addItem(tr("string"),     QVariant("/j"));
    ui->searchspaceCombo->addItem(tr("hex string"), QVariant("/xj"));
    ui->searchspaceCombo->addItem(tr("ROP gadgets"), QVariant("/Rj"));
    ui->searchspaceCombo->addItem(tr("32bit value"), QVariant("/vj"));

    if (cur_idx > 0)
        ui->searchspaceCombo->setCurrentIndex(cur_idx);

    refreshSearch();
}

void SearchWidget::refreshSearch()
{
    QString search_for = ui->filterLineEdit->text();
    QVariant searchspace_data = ui->searchspaceCombo->currentData();
    QString searchspace = searchspace_data.toString();

    search_model->beginResetModel();
    search = Core()->getAllSearch(search_for, searchspace);
    search_model->endResetModel();

    qhelpers::adjustColumns(ui->searchTreeView, 3, 0);
}

void SearchWidget::setScrollMode()
{
    qhelpers::setVerticalScrollMode(ui->searchTreeView);
}

void SearchWidget::updatePlaceholderText(int index)
{
    switch (index) {
    case 1: // string
        ui->filterLineEdit->setPlaceholderText("foobar");
        break;
    case 2: // hex string
        ui->filterLineEdit->setPlaceholderText("deadbeef");
        break;
    case 3: // ROP gadgets
        ui->filterLineEdit->setPlaceholderText("pop,,pop");
        break;
    case 4: // 32bit value
        ui->filterLineEdit->setPlaceholderText("0xdeadbeef");
        break;
    default:
        ui->filterLineEdit->setPlaceholderText("jmp rax");
    }
}


void SearchWidget::on_searchInCombo_currentIndexChanged(int index)
{
    Config()->setConfig("search.in",
                      ui->searchInCombo->itemData(index).toString());
}
