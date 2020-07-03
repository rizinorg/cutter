#include "CallGraph.h"

#include "MainWindow.h"

#include <QJsonValue>
#include <QJsonArray>
#include <QJsonObject>

CallGraphWidget::CallGraphWidget(MainWindow *main, bool global)
    : CutterDockWidget(main)
    , graphView(new CallGraphView(this, main, global))
{
    setObjectName(main->getUniqueObjectName("CallGraphWidget"));
    this->setWindowTitle(global ? tr("Global Callgraph") : tr("Callgraph"));

    setWidget(graphView);
    connect(Core(), &CutterCore::refreshAll, this, [this]() {
        graphView->refreshView();
    });
}

CallGraphWidget::~CallGraphWidget()
{
}

CallGraphView::CallGraphView(QWidget *parent, MainWindow *main, bool global)
    : SimpleTextGraphView(parent, main)
    , global(global)
{
}

void CallGraphView::loadCurrentGraph()
{
    blockContent.clear();
    blocks.clear();

    QJsonDocument functionsDoc = Core()->cmdj(global ? "agCj" : "agcj");
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

        addBlock(std::move(layoutBlock), name);
    }
    for (auto it = idMapping.begin(), end = idMapping.end(); it != end; ++it) {
        if (blocks.find(it.value()) == blocks.end()) {
            GraphLayout::GraphBlock block;
            block.entry = it.value();
            addBlock(std::move(block), it.key());
        }
    }

    computeGraphPlacement();
}

