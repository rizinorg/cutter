#include "SearchWidget.h"

#include "DisassemblyPreview.h"
#include "common/Helpers.h"
#include "core/MainWindow.h"
#include "ui_SearchWidget.h"

#include <QComboBox>
#include <QDockWidget>
#include <QShortcut>
#include <QTreeWidget>

namespace {
const int kMaxTooltipWidth = 500;
const int kMaxTooltipDisasmPreviewLines = 10;
const int kMaxTooltipHexdumpBytes = 64;
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
    if (index.row() >= search.count()) {
        return QVariant();
    }

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
            return rzAddressString(exp.offset);
        case SIZE:
            return rzSizeString(exp.size);
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
        const QFontMetrics fm { fnt };

        QString toolTipContent =
                QString("<html><div style=\"font-family: '%1'; font-size: %2pt; white-space: "
                        "nowrap;\">")
                        .arg(fnt.family().toHtmlEscaped())
                        .arg(qMax(6, fnt.pointSize() - 1)); // slightly decrease font size, to keep
                                                            // more text in the same box

        toolTipContent +=
                tr("<div style=\"margin-bottom: 10px;\"><strong>Preview</strong>:<br>%1</div>")
                        .arg(previewContent);

        toolTipContent += "</div></html>";
        return toolTipContent;
    }
    case searchDescriptionRole:
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
    const QModelIndex index = sourceModel()->index(row, 0, parent);
    const auto search = index.data(SearchModel::searchDescriptionRole).value<SearchDescription>();
    return qhelpers::filterStringContains(search.code, this);
}

bool SearchSortFilterProxyModel::lessThan(const QModelIndex &left, const QModelIndex &right) const
{
    const auto leftSearch =
            left.data(SearchModel::searchDescriptionRole).value<SearchDescription>();
    const auto rightSearch =
            right.data(SearchModel::searchDescriptionRole).value<SearchDescription>();

    switch (left.column()) {
    case SearchModel::SIZE:
        return leftSearch.size < rightSearch.size;
    case SearchModel::OFFSET:
        return leftSearch.offset < rightSearch.offset;
    case SearchModel::CODE:
        return leftSearch.code < rightSearch.code;
    case SearchModel::DATA:
        return leftSearch.data < rightSearch.data;
    case SearchModel::COMMENT:
        return leftSearch.detail < rightSearch.detail;
    default:
        break;
    }

    return leftSearch.offset < rightSearch.offset;
}

SearchWidget::SearchWidget(MainWindow *main)
    : CutterDockWidget(main),
      ui(new Ui::SearchWidget),
      searchModel(new SearchModel(this)),
      searchProxyModel(new SearchSortFilterProxyModel(searchModel, this))
{
    ui->setupUi(this);
    setStyleSheet(QString("QToolTip { max-width: %1px; opacity: 230; }").arg(kMaxTooltipWidth));

    updateSearchBoundaries();

    ui->searchTreeView->setModel(static_cast<AddressableItemModelI *>(searchProxyModel));
    ui->searchTreeView->setMainWindow(main);
    ui->searchTreeView->sortByColumn(SearchModel::OFFSET, Qt::AscendingOrder);

    setScrollMode();

    connect(Core(), &CutterCore::toggleDebugView, this, &SearchWidget::updateSearchBoundaries);
    connect(Core(), &CutterCore::refreshAll, this, &SearchWidget::refreshSearchspaces);
    connect(Core(), &CutterCore::commentsChanged, this,
            [this]() { qhelpers::emitColumnChanged(searchModel, SearchModel::COMMENT); });

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
    for (const auto &item : boundaries) {
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
    int curIdx = ui->searchspaceCombo->currentIndex();
    if (curIdx < 0) {
        curIdx = 0;
    }

    ui->searchspaceCombo->clear();
    for (auto &kind : searchKinds) {
        ui->searchspaceCombo->addItem(tr(kind.name), static_cast<int>(kind.kind));
    }

    if (curIdx > 0) {
        ui->searchspaceCombo->setCurrentIndex(curIdx);
    }

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
    const QString searchFor = ui->filterLineEdit->text();
    auto searchSpace = static_cast<SearchKind>(ui->searchspaceCombo->currentData().toInt());
    const QString searchIn = ui->searchInCombo->currentData().toString();

    searchModel->beginResetModel();
    searchModel->search = Core()->getAllSearch(searchFor, searchSpace, searchIn);
    searchModel->endResetModel();

    qhelpers::adjustColumns(ui->searchTreeView, 3, 0);
}

// No Results Found information message when search returns empty
// Called by &QShortcut::activated and &QAbstractButton::clicked signals
void SearchWidget::checkSearchResultEmpty()
{
    if (!searchModel->search.isEmpty()) {
        return;
    }

    const QString searchFor = ui->filterLineEdit->text();
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
