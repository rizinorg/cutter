#include "GenericR2GraphView.h"

#include "core/Cutter.h"
#include <QJsonValue>
#include <QJsonArray>
#include <QJsonObject>

void GenericR2GraphView::loadCurrentGraph()
{
    blockContent.clear();
    blocks.clear();

    QChar graphType = 'r';

    QJsonDocument functionsDoc = Core()->cmdj(QString("ag%1j").arg(graphType));
    auto nodes = functionsDoc["nodes"].toArray();

    for (const QJsonValueRef &value : nodes) {
        QJsonObject block = value.toObject();
        uint64_t id = block["id"].toVariant().toULongLong();

        QString content;
        QString title = block["title"].toString();
        QString body = block["body"].toString();
        if (!title.isEmpty() && !body.isEmpty()) {
            content = title + "/n" + body;
        } else {
            content = title + body;
        }

        auto edges = block["out_nodes"].toArray();
        GraphLayout::GraphBlock layoutBlock;
        layoutBlock.entry = id;
        for (auto edge : edges) {
            auto targetId = edge.toVariant().toULongLong();
            layoutBlock.edges.emplace_back(targetId);
        }

        blockContent[id] = {content};
        prepareGraphNode(layoutBlock);
        addBlock(layoutBlock);
    }

    computeGraphPlacement();
}
