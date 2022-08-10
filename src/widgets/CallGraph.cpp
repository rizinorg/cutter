#include "CallGraph.h"

#include "MainWindow.h"

#include <QJsonValue>
#include <QJsonArray>
#include <QJsonObject>

CallGraphWidget::CallGraphWidget(MainWindow *main, bool global)
    : AddressableDockWidget(main), graphView(new CallGraphView(this, main, global)), global(global)
{
    setObjectName(main->getUniqueObjectName("CallGraphWidget"));
    this->setWindowTitle(getWindowTitle());
    connect(seekable, &CutterSeekable::seekableSeekChanged, this, &CallGraphWidget::onSeekChanged);

    setWidget(graphView);
}

CallGraphWidget::~CallGraphWidget() {}

QString CallGraphWidget::getWindowTitle() const
{
    return global ? tr("Global Callgraph") : tr("Callgraph");
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
    refreshDeferrer.registerFor(parent);
    connect(&refreshDeferrer, &RefreshDeferrer::refreshNow, this, &CallGraphView::refreshView);
    connect(Core(), &CutterCore::refreshAll, this, &SimpleTextGraphView::refreshView);
}

void CallGraphView::showExportDialog()
{
    QString defaultName;
    if (global) {
        defaultName = "global_callgraph";
    } else {
        defaultName = QString("callgraph_%1").arg(RzAddressString(address));
    }
    showExportGraphDialog(defaultName, global ? "agC" : "agc", address);
}

void CallGraphView::showAddress(RVA address)
{
    if (global) {
        auto addressMappingIt = addressMapping.find(address);
        if (addressMappingIt != addressMapping.end()) {
            selectBlockWithId(addressMappingIt->second);
            showBlock(blocks[addressMappingIt->second]);
        }
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

static inline bool clamp(ut64 a, ut64 x, ut64 b)
{
    return (a == UT64_MAX || a <= x) && (b == UT64_MAX || x <= b);
}

using PRzList = std::unique_ptr<RzList, decltype(rz_list_free) *>;

void CallGraphView::addFunction(RzAnalysisFunction *fcn,
                                const std::function<ut64(const QString &)> &getId)
{
    static const auto xrefCmp = [](const void *a, const void *b) -> int {
        auto x = reinterpret_cast<const RzAnalysisXRef *>(a)->to
                - reinterpret_cast<const RzAnalysisXRef *>(b)->to;
        return static_cast<int>(x);
    };
    const bool usenames = Core()->getConfigb("graph.json.usenames");

    QString name = usenames ? fcn->name : RzAddressString(fcn->addr);
    GraphLayout::GraphBlock block;
    block.entry = getId(name);

    auto xrefs = PRzList { rz_analysis_function_get_xrefs_from(fcn), rz_list_free };
    auto calls = PRzList { rz_list_new(), rz_list_free };
    for (const auto &xref : CutterRzList<RzAnalysisXRef>(xrefs.get())) {
        if (!(xref->type == RZ_ANALYSIS_XREF_TYPE_CALL
              && rz_list_find(calls.get(), xref, (RzListComparator)xrefCmp) == NULL)) {
            continue;
        }
        rz_list_append(calls.get(), xref);
        RzFlagItem *flag = rz_flag_get_i(Core()->core()->flags, xref->to);
        QString xref_name = usenames
                ? ((flag && flag->name) ? flag->name
                                        : QString("unk.%0").arg(RzAddressString(xref->to)))
                : RzAddressString(xref->to);
        block.edges.emplace_back(getId(xref_name));
    }

    addBlock(std::move(block), name, Core()->num(name));
}

void CallGraphView::loadCurrentGraph()
{
    blockContent.clear();
    blocks.clear();

    const ut64 from = Core()->getConfigi("graph.from");
    const ut64 to = Core()->getConfigi("graph.to");
    QHash<QString, uint64_t> idMapping;
    auto getId = [&](const QString &name) -> uint64_t {
        auto nextId = idMapping.size();
        auto &itemId = idMapping[name];
        if (idMapping.size() != nextId) {
            itemId = nextId;
        }
        return itemId;
    };

    if (global) {
        for (const auto &fcn : CutterRzList<RzAnalysisFunction>(Core()->core()->analysis->fcns)) {
            if (!clamp(from, fcn->addr, to)) {
                continue;
            }
            addFunction(fcn, getId);
        }
    } else {
        const auto &fcn = Core()->functionIn(address);
        if (fcn) {
            addFunction(fcn, getId);
        }
    }

    for (auto it = idMapping.constBegin(), end = idMapping.constEnd(); it != end; ++it) {
        const auto &k = it.key();
        const auto &v = it.value();
        if (blocks.find(v) != blocks.end())
            continue;
        GraphLayout::GraphBlock block;
        block.entry = v;
        addBlock(std::move(block), k, Core()->num(k));
    }
    if (blockContent.empty() && !global) {
        addBlock({}, RzAddressString(address), address);
    }

    addressMapping.clear();
    for (const auto &it : blockContent) {
        addressMapping[it.second.address] = it.first;
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
