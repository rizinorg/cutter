#include "SearchWidget.h"
#include "ui_SearchWidget.h"
#include "core/MainWindow.h"
#include "common/Helpers.h"
#include "DisassemblyPreview.h"

#include <QDockWidget>
#include <QTreeWidget>
#include <QComboBox>
#include <QShortcut>

namespace {

static const int kMaxTooltipWidth = 500;
static const int kMaxTooltipDisasmPreviewLines = 10;
static const int kMaxTooltipHexdumpBytes = 64;

}

static const QVector<std::pair<QString, const char *>> searchBoundaries {
    { "io.maps", QT_TRANSLATE_NOOP("SearchWidget", "All maps") },
    { "io.map", QT_TRANSLATE_NOOP("SearchWidget", "Current map") },
    { "raw", QT_TRANSLATE_NOOP("SearchWidget", "Whole file") },
    { "block", QT_TRANSLATE_NOOP("SearchWidget", "Current block") },
    { "bin.section", QT_TRANSLATE_NOOP("SearchWidget", "Current mapped section") },
    { "bin.sections", QT_TRANSLATE_NOOP("SearchWidget", "All mapped sections") },
    { "bin.segment", QT_TRANSLATE_NOOP("SearchWidget", "Current mapped segment") },
    { "bin.segments", QT_TRANSLATE_NOOP("SearchWidget", "All mapped segments") },
    { "code", QT_TRANSLATE_NOOP("SearchWidget", "All exec sections") },
    { "io.sky", QT_TRANSLATE_NOOP("SearchWidget", "All io.skyline") },
    { "analysis.fcn", QT_TRANSLATE_NOOP("SearchWidget", "Current function") },
    { "analysis.bb", QT_TRANSLATE_NOOP("SearchWidget", "Current basic block") },
};

static const QVector<std::pair<QString, const char *>> searchBoundariesDebug {
    { "dbg.maps", QT_TRANSLATE_NOOP("SearchWidget", "All memory maps") },
    { "dbg.map", QT_TRANSLATE_NOOP("SearchWidget", "Memory map") },
    { "block", QT_TRANSLATE_NOOP("SearchWidget", "Current block") },
    { "dbg.program", QT_TRANSLATE_NOOP("SearchWidget", "All exec sections") },
    { "dbg.stack", QT_TRANSLATE_NOOP("SearchWidget", "Stack") },
    { "dbg.heap", QT_TRANSLATE_NOOP("SearchWidget", "Heap") }
};

struct SearchKindInfo
{
    SearchKind kind;
    const char *name;
    const char *textHint;
    bool noInput;
};

static const SearchKindInfo searchKinds[] = {
    { SearchKind::AsmCode, QT_TRANSLATE_NOOP("SearchWidget", "asm code"),
      QT_TRANSLATE_NOOP("SearchWidget", "jmp rax"), false },
    { SearchKind::String, QT_TRANSLATE_NOOP("SearchWidget", "string (literal)"),
      QT_TRANSLATE_NOOP("SearchWidget", "foobar"), false },
    { SearchKind::StringCaseInsensitive,
      QT_TRANSLATE_NOOP("SearchWidget", "string (case insensitive)"),
      QT_TRANSLATE_NOOP("SearchWidget", "fOobaR"), false },
    { SearchKind::StringRegexExtended, QT_TRANSLATE_NOOP("SearchWidget", "string (extended regex)"),
      QT_TRANSLATE_NOOP("SearchWidget", "(foo){,4}[Bb]ar"), false },
    { SearchKind::HexString, QT_TRANSLATE_NOOP("SearchWidget", "hex string"),
      QT_TRANSLATE_NOOP("SearchWidget", "ab01..23...1234ef"), false },
    { SearchKind::ROPGadgets, QT_TRANSLATE_NOOP("SearchWidget", "ROP gadgets"),
      QT_TRANSLATE_NOOP("SearchWidget", "pop,,pop"), false },
    { SearchKind::ROPGadgetsRegex, QT_TRANSLATE_NOOP("SearchWidget", "ROP gadgets (regex)"),
      QT_TRANSLATE_NOOP("SearchWidget", "mov e[abc]x"), false },
    { SearchKind::Value32BE, QT_TRANSLATE_NOOP("SearchWidget", "32bit big endian value"),
      QT_TRANSLATE_NOOP("SearchWidget", "0xdeadbeef (big endian)"), false },
    { SearchKind::Value32LE, QT_TRANSLATE_NOOP("SearchWidget", "32bit little endian value"),
      QT_TRANSLATE_NOOP("SearchWidget", "0xdeadbeef (little endian)"), false },
    { SearchKind::Value64BE, QT_TRANSLATE_NOOP("SearchWidget", "64bit big endian value"),
      QT_TRANSLATE_NOOP("SearchWidget", "0xfedcba9876543210 (big endian)"), false },
    { SearchKind::Value64BE, QT_TRANSLATE_NOOP("SearchWidget", "64bit little endian value"),
      QT_TRANSLATE_NOOP("SearchWidget", "0xfedcba9876543210 (little endian)"), false },
    { SearchKind::CryptographicMaterial,
      QT_TRANSLATE_NOOP("SearchWidget", "Cryptographic material"), nullptr, true },
    { SearchKind::MagicSignature, QT_TRANSLATE_NOOP("SearchWidget", "Magic signature"), nullptr,
      true },
};

static const SearchKindInfo &searchKindInfo(SearchKind kind)
{
    auto res = std::find_if(std::begin(searchKinds), std::end(searchKinds),
                            [kind](const SearchKindInfo &info) { return info.kind == kind; });
    if (res != std::end(searchKinds)) {
        return *res;
    }
    return searchKinds[1];
}

SearchModel::SearchModel(QObject *parent) : AddressableItemModel<QAbstractListModel>(parent) {}

int SearchModel::rowCount(const QModelIndex &) const
{
    return search.count();
}

int SearchModel::columnCount(const QModelIndex &) const
{
    return Columns::COUNT;
}

QVariant SearchModel::data(const QModelIndex &index, int role) const
{
    if (index.row() >= search.count())
        return QVariant();

    const SearchDescription &exp = search.at(index.row());

    switch (role) {
    case Qt::FontRole: {
        switch (index.column()) {
        case CODE:
            return QFont("Inconsolata");
        case DATA:
            return QFont("Inconsolata");
        default:
            return QVariant();
        }
    }
    case Qt::DisplayRole:
        switch (index.column()) {
        case OFFSET:
            return RzAddressString(exp.offset);
        case SIZE:
            return RzSizeString(exp.size);
        case CODE:
            return exp.code;
        case DATA:
            return exp.data;
        case COMMENT: {
            return exp.detail;
        }
        default:
            return QVariant();
        }
    case Qt::ToolTipRole: {

        QString previewContent = QString();
        // if result is CODE, show disassembly
        if (!exp.code.isEmpty()) {
            previewContent =
                    Core()->getDisassemblyPreview(exp.offset, kMaxTooltipDisasmPreviewLines)
                            .join("<br>");
            // if result is DATA or Disassembly is N/A
        } else if (!exp.data.isEmpty() || previewContent.isEmpty()) {
            previewContent = Core()->getHexdumpPreview(exp.offset, kMaxTooltipHexdumpBytes);
        }

        const QFont &fnt = Config()->getBaseFont();
        QFontMetrics fm { fnt };

        QString toolTipContent =
                QString("<html><div style=\"font-family: %1; font-size: %2pt; white-space: "
                        "nowrap;\">")
                        .arg(fnt.family())
                        .arg(qMax(6, fnt.pointSize() - 1)); // slightly decrease font size, to keep
                                                            // more text in the same box

        toolTipContent +=
                tr("<div style=\"margin-bottom: 10px;\"><strong>Preview</strong>:<br>%1</div>")
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
        case COMMENT:
            return tr("Comment");
        default:
            return QVariant();
        }
    default:
        return QVariant();
    }
}

RVA SearchModel::address(const QModelIndex &index) const
{
    const SearchDescription &exp = search.at(index.row());
    return exp.offset;
}

SearchSortFilterProxyModel::SearchSortFilterProxyModel(SearchModel *source_model, QObject *parent)
    : AddressableFilterProxyModel(source_model, parent)
{
}

bool SearchSortFilterProxyModel::filterAcceptsRow(int row, const QModelIndex &parent) const
{
    QModelIndex index = sourceModel()->index(row, 0, parent);
    SearchDescription search =
            index.data(SearchModel::SearchDescriptionRole).value<SearchDescription>();
    return qhelpers::filterStringContains(search.code, this);
}

bool SearchSortFilterProxyModel::lessThan(const QModelIndex &left, const QModelIndex &right) const
{
    SearchDescription left_search =
            left.data(SearchModel::SearchDescriptionRole).value<SearchDescription>();
    SearchDescription right_search =
            right.data(SearchModel::SearchDescriptionRole).value<SearchDescription>();

    switch (left.column()) {
    case SearchModel::SIZE:
        return left_search.size < right_search.size;
    case SearchModel::OFFSET:
        return left_search.offset < right_search.offset;
    case SearchModel::CODE:
        return left_search.code < right_search.code;
    case SearchModel::DATA:
        return left_search.data < right_search.data;
    case SearchModel::COMMENT:
        return left_search.detail < right_search.detail;
    default:
        break;
    }

    return left_search.offset < right_search.offset;
}

SearchWidget::SearchWidget(MainWindow *main) : CutterDockWidget(main), ui(new Ui::SearchWidget)
{
    ui->setupUi(this);
    setStyleSheet(QString("QToolTip { max-width: %1px; opacity: 230; }").arg(kMaxTooltipWidth));

    updateSearchBoundaries();

    search_model = new SearchModel(this);
    search_proxy_model = new SearchSortFilterProxyModel(search_model, this);
    ui->searchTreeView->setModel(search_proxy_model);
    ui->searchTreeView->setMainWindow(main);
    ui->searchTreeView->sortByColumn(SearchModel::OFFSET, Qt::AscendingOrder);

    setScrollMode();

    connect(Core(), &CutterCore::toggleDebugView, this, &SearchWidget::updateSearchBoundaries);
    connect(Core(), &CutterCore::refreshAll, this, &SearchWidget::refreshSearchspaces);
    connect(Core(), &CutterCore::commentsChanged, this,
            [this]() { qhelpers::emitColumnChanged(search_model, SearchModel::COMMENT); });

    connect(ui->filterLineEdit, &QLineEdit::returnPressed, this, &SearchWidget::runSearch);
    connect(ui->searchButton, &QAbstractButton::clicked, this, &SearchWidget::runSearch);

    connect(ui->searchspaceCombo,
            static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged), this,
            [this](int index) { updatePlaceholderText(index); });

    updateColors();
    connect(Config(), &Configuration::colorsUpdated, this, &SearchWidget::updateColors);
}

SearchWidget::~SearchWidget() {}

void SearchWidget::updateSearchBoundaries()
{
    QVector<std::pair<QString, const char *>> boundaries;

    if (Core()->currentlyDebugging && !Core()->currentlyEmulating) {
        boundaries = searchBoundariesDebug;
    } else {
        boundaries = searchBoundaries;
    }

    ui->searchInCombo->setCurrentIndex(ui->searchInCombo->findData(boundaries[0].first));

    ui->searchInCombo->blockSignals(true);
    ui->searchInCombo->clear();
    for (auto item : boundaries) {
        ui->searchInCombo->addItem(tr(item.second), item.first);
    }
    ui->searchInCombo->blockSignals(false);

    ui->filterLineEdit->clear();
}

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
    for (auto &kind : searchKinds) {
        ui->searchspaceCombo->addItem(tr(kind.name), static_cast<int>(kind.kind));
    }

    if (cur_idx > 0)
        ui->searchspaceCombo->setCurrentIndex(cur_idx);

    refreshSearch();
}

void SearchWidget::runSearch()
{
    disableSearch();
    refreshSearch();
    checkSearchResultEmpty();
    enableSearch();
}

void SearchWidget::refreshSearch()
{
    QString searchFor = ui->filterLineEdit->text();
    auto searchSpace = static_cast<SearchKind>(ui->searchspaceCombo->currentData().toInt());
    QString searchIn = ui->searchInCombo->currentData().toString();

    search_model->beginResetModel();
    search_model->search = Core()->getAllSearch(searchFor, searchSpace, searchIn);
    search_model->endResetModel();

    qhelpers::adjustColumns(ui->searchTreeView, 3, 0);
}

// No Results Found information message when search returns empty
// Called by &QShortcut::activated and &QAbstractButton::clicked signals
void SearchWidget::checkSearchResultEmpty()
{
    if (!search_model->search.isEmpty())
        return;

    QString searchFor = ui->filterLineEdit->text();
    auto searchSpace = static_cast<SearchKind>(ui->searchspaceCombo->currentData().toInt());
    if (searchFor.isEmpty() && !searchKindInfo(searchSpace).noInput) {
        return;
    }
    QString noResultsMessage = "<b>";
    noResultsMessage.append(tr("No results found for:"));
    noResultsMessage.append("</b><br>");
    if (searchFor.isEmpty()) {
        noResultsMessage.append(ui->searchspaceCombo->currentText().toHtmlEscaped());
    } else {
        noResultsMessage.append(ui->filterLineEdit->text().toHtmlEscaped());
    }

    QMessageBox::information(this, tr("No Results Found"), noResultsMessage);
}

void SearchWidget::setScrollMode()
{
    qhelpers::setVerticalScrollMode(ui->searchTreeView);
}

void SearchWidget::updatePlaceholderText(int)
{
    // ensure we grab the correct kind.
    auto kind = static_cast<SearchKind>(ui->searchspaceCombo->currentData().toInt());
    auto info = searchKindInfo(kind);
    if (info.textHint) {
        ui->filterLineEdit->setPlaceholderText(tr(info.textHint));
    } else {
        ui->filterLineEdit->setPlaceholderText("");
    }
    ui->filterLineEdit->setDisabled(info.noInput);
    if (info.noInput) {
        ui->filterLineEdit->clear();
    }
}

void SearchWidget::disableSearch()
{
    ui->searchButton->setEnabled(false);
    ui->searchButton->setText(tr("Searching..."));
    qApp->processEvents();
}

void SearchWidget::enableSearch()
{
    ui->searchButton->setEnabled(true);
    ui->searchButton->setText(tr("Search"));
}

void SearchWidget::updateColors()
{
    ui->searchTreeView->setStyleSheet(DisassemblyPreview::getToolTipStyleSheet());
}
