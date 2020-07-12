#include "CallGraph.h"

#include "MainWindow.h"

#include <QJsonValue>
#include <QJsonArray>
#include <QJsonObject>

CallGraphWidget::CallGraphWidget(MainWindow *main, bool global)
    : AddressableDockWidget(main)
    , graphView(new CallGraphView(this, main, global))
    , global(global)
{
    setObjectName(main->getUniqueObjectName("CallGraphWidget"));
    this->setWindowTitle(getWindowTitle());
    connect(seekable, &CutterSeekable::seekableSeekChanged, this, &CallGraphWidget::onSeekChanged);

    setWidget(graphView);
}

CallGraphWidget::~CallGraphWidget()
{
}

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
    : SimpleTextGraphView(parent, main)
    , global(global)
    , refreshDeferrer(nullptr, this)
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
        defaultName = QString("callgraph_%1").arg(RAddressString(address));
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

void CallGraphView::loadCurrentGraph()
{
    blockContent.clear();
    blocks.clear();

    QJsonDocument functionsDoc = Core()->cmdj(global ? "agCj" : QString("agcj @ %1").arg(address));
    auto nodes = functionsDoc.array();

    QHash<QString, uint64_t> idMapping;

    auto getId = [&](const QString &name) -> uint64_t {
        auto nextId = idMapping.size();
        auto &itemId = idMapping[name];
        if (idMapping.size() != nextId) {
            itemId = nextId;
        }
        return itemId;
    };

    for (const QJsonValueRef &value : nodes) {
        QJsonObject block = value.toObject();
        QString name = block["name"].toVariant().toString();

        auto edges = block["imports"].toArray();
        GraphLayout::GraphBlock layoutBlock;
        layoutBlock.entry = getId(name);
        for (auto edge : edges) {
            auto targetName = edge.toString();
            auto targetId = getId(targetName);
            layoutBlock.edges.emplace_back(targetId);
        }

        // it would be good if address came directly from json instead of having to lookup by name
        addBlock(std::move(layoutBlock), name, Core()->num(name));
    }
    for (auto it = idMapping.begin(), end = idMapping.end(); it != end; ++it) {
        if (blocks.find(it.value()) == blocks.end()) {
            GraphLayout::GraphBlock block;
            block.entry = it.value();
            addBlock(std::move(block), it.key(), Core()->num(it.key()));
        }
    }
    if (blockContent.empty() && !global) {
        addBlock({}, RAddressString(address), address);
    }

    addressMapping.clear();
    for (auto &it : blockContent) {
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
