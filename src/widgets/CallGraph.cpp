#include "CallGraph.h"

#include "MainWindow.h"

#include <QJsonValue>
#include <QJsonArray>
#include <QJsonObject>

CallGraphWidget::CallGraphWidget(MainWindow *main, bool global)
    : MemoryDockWidget(MemoryWidgetType::CallGraph, main),
      graphView(new CallGraphView(this, main, global)),
      global(global)
{
    setObjectName(main ? main->getUniqueObjectName(getWidgetType()) : getWidgetType());
    this->setWindowTitle(getWindowTitle());
    connect(seekable, &CutterSeekable::seekableSeekChanged, this, &CallGraphWidget::onSeekChanged);

    setWidget(graphView);
}

CallGraphWidget::~CallGraphWidget() {}

QString CallGraphWidget::getWindowTitle() const
{
    return global ? tr("Global Callgraph") : tr("Callgraph");
}

QString CallGraphWidget::getWidgetType() const
{
    return global ? tr("GlobalCallgraph") : tr("Callgraph");
}

void CallGraphWidget::onSeekChanged(RVA address)
{
    if (auto function = Core()->functionIn(address)) {
        graphView->showAddress(function->addr);
    }
}

CallGraphView::CallGraphView(CutterDockWidget *parent, MainWindow *main, bool global)
    : SimpleTextGraphView(parent, main), global(global), refreshDeferrer(nullptr, this)
{
    enableAddresses(true);
    addressableItemContextMenu.toggleBreakpointAction(true);
    refreshDeferrer.registerFor(parent);
    connect(&refreshDeferrer, &RefreshDeferrer::refreshNow, this, &CallGraphView::refreshView);
    connect(Core(), &CutterCore::refreshAll, this, &SimpleTextGraphView::refreshView);
    connect(Core(), &CutterCore::functionRenamed, this, &CallGraphView::refreshView);
}

void CallGraphView::showExportDialog()
{
    QString defaultName;
    if (global) {
        defaultName = "global_callgraph";
    } else {
        defaultName = QString("callgraph_%1").arg(RzAddressString(address));
    }
    showExportGraphDialog(defaultName, RZ_CORE_GRAPH_TYPE_FUNCALL, global ? RVA_INVALID : address);
}

void CallGraphView::showAddress(RVA address)
{
    if (global) {
        selectBlockWithId(address);
        showBlock(blocks[address]);
    } else if (address != this->address) {
        this->address = address;
        refreshView();
    }
}

void CallGraphView::refreshView()
{
    if (!refreshDeferrer.attemptRefresh(nullptr)) {
        return;
    }
    SimpleTextGraphView::refreshView();
}

static inline bool isBetween(ut64 a, ut64 x, ut64 b)
{
    return (a == UT64_MAX || a <= x) && (b == UT64_MAX || x <= b);
}

void CallGraphView::loadCurrentGraph()
{
    blockContent.clear();
    blocks.clear();

    const ut64 from = Core()->getConfigi("graph.from");
    const ut64 to = Core()->getConfigi("graph.to");
    const bool usenames = Core()->getConfigb("graph.json.usenames");

    auto edges = std::unordered_set<ut64> {};
    auto addFunction = [&](RzAnalysisFunction *fcn) {
        GraphLayout::GraphBlock block;
        block.entry = fcn->addr;

        auto xrefs = fromOwned(rz_analysis_function_get_xrefs_from(fcn));
        auto calls = std::unordered_set<ut64>();
        for (const auto &xref : CutterRzList<RzAnalysisXRef>(xrefs.get())) {
            const auto x = xref->to;
            if (!(xref->type == RZ_ANALYSIS_XREF_TYPE_CALL && calls.find(x) == calls.end())) {
                continue;
            }
            calls.insert(x);
            block.edges.emplace_back(x);
            edges.insert(x);
        }

        QString name = usenames ? fcn->name : RzAddressString(fcn->addr);
        addBlock(std::move(block), name, fcn->addr);
    };

    auto core = Core()->lock();
    if (global) {
        for (const auto &fcn : CutterRzList<RzAnalysisFunction>(core->analysis->fcns)) {
            if (!isBetween(from, fcn->addr, to)) {
                continue;
            }
            addFunction(fcn);
        }
    } else {
        const auto &fcn = Core()->functionIn(address);
        if (fcn) {
            addFunction(fcn);
        }
    }

    for (const auto &x : edges) {
        if (blockContent.find(x) != blockContent.end()) {
            continue;
        }
        GraphLayout::GraphBlock block;
        block.entry = x;
        QString flagName = Core()->flagAt(x);
        QString name = usenames
                ? (!flagName.isEmpty() ? flagName : QString("unk.%0").arg(RzAddressString(x)))
                : RzAddressString(x);
        addBlock(std::move(block), name, x);
    }
    if (blockContent.empty() && !global) {
        const auto name = RzAddressString(address);
        addBlock({}, name, address);
    }

    computeGraphPlacement();
}

void CallGraphView::restoreCurrentBlock()
{
    if (!global && lastLoadedAddress != address) {
        selectedBlock = NO_BLOCK_SELECTED;
        lastLoadedAddress = address;
        center();
    } else {
        SimpleTextGraphView::restoreCurrentBlock();
    }
}
