#include "RizinGraphWidget.h"

#include "ui_RizinGraphWidget.h"

#include <QJsonArray>
#include <QJsonObject>
#include <QJsonValue>

#include <utility>

RizinGraphWidget::RizinGraphWidget(MainWindow *main)
    : CutterDockWidget(main),
      ui(new Ui::RizinGraphWidget),
      graphView(new GenericRizinGraphView(this, main))
{
    ui->setupUi(this);
    ui->verticalLayout->addWidget(graphView);
    connect(ui->refreshButton, &QPushButton::pressed, this, [this]() { graphView->refreshView(); });
    struct GraphType
    {
        QChar commandChar;
        QString label;
    } types[] = {
        { 'a', tr("Data reference graph (aga)") },
        { 'A', tr("Global data references graph (agA)") },
        // {'c', tr("c - Function callgraph")},
        // {'C', tr("C - Global callgraph")},
        // {'f', tr("f - Basic blocks function graph")},
        { 'i', tr("Imports graph (agi)") },
        { 'r', tr("References graph (agr)") },
        { 'R', tr("Global references graph (agR)") },
        { 'x', tr("Cross references graph (agx)") },
        { 'I', tr("RzIL statement graph (agI)") },
        { 'g', tr("Custom graph (agg)") },
        { ' ', tr("User command") },
    };
    for (auto &graphType : types) {
        if (graphType.commandChar != ' ') {
            ui->graphType->addItem(graphType.label, graphType.commandChar);
        } else {
            ui->graphType->addItem(graphType.label, QVariant());
        }
    }
    connect<void (QComboBox::*)(int)>(ui->graphType, &QComboBox::currentIndexChanged, this,
                                      &RizinGraphWidget::typeChanged);
    connect(ui->customCommand, &QLineEdit::textEdited, this,
            [this]() { graphView->setGraphCommand(ui->customCommand->text()); });
    connect(ui->customCommand, &QLineEdit::returnPressed, this, [this]() {
        graphView->setGraphCommand(ui->customCommand->text());
        graphView->refreshView();
    });
    ui->customCommand->hide();
    typeChanged();
}

RizinGraphWidget::~RizinGraphWidget() {}

void RizinGraphWidget::typeChanged()
{
    auto currentData = ui->graphType->currentData();
    if (currentData.isNull()) {
        ui->customCommand->setVisible(true);
        graphView->setGraphCommand(ui->customCommand->text());
        ui->customCommand->setFocus();
    } else {
        ui->customCommand->setVisible(false);
        auto command = QString("ag%1").arg(currentData.toChar());
        graphView->setGraphCommand(command);
        graphView->refreshView();
    }
}

GenericRizinGraphView::GenericRizinGraphView(RizinGraphWidget *parent, MainWindow *main)
    : SimpleTextGraphView(parent, main), refreshDeferrer(nullptr, this)
{
    refreshDeferrer.registerFor(parent);
    connect(&refreshDeferrer, &RefreshDeferrer::refreshNow, this,
            &GenericRizinGraphView::refreshView);
}

void GenericRizinGraphView::setGraphCommand(QString cmd)
{
    graphCommand = std::move(cmd);
}

void GenericRizinGraphView::refreshView()
{
    if (!refreshDeferrer.attemptRefresh(nullptr)) {
        return;
    }
    SimpleTextGraphView::refreshView();
}

void GenericRizinGraphView::loadCurrentGraph()
{
    blockContent.clear();
    blocks.clear();

    if (graphCommand.isEmpty()) {
        return;
    }

    const CutterJson functionsDoc = Core()->cmdj(QString("%1 json").arg(graphCommand));
    auto nodes = functionsDoc["nodes"];

    for (const CutterJson block : nodes) {
        const uint64_t id = block["id"].toUt64();

        QString content;
        const QString title = block["title"].toString();
        const QString body = block["body"].toString();
        if (!title.isEmpty() && !body.isEmpty()) {
            content = title + "/n" + body;
        } else {
            content = title + body;
        }

        auto edges = block["out_nodes"];
        GraphLayout::GraphBlock layoutBlock;
        layoutBlock.entry = id;
        for (auto edge : edges) {
            auto targetId = edge.toUt64();
            layoutBlock.edges.emplace_back(targetId);
        }

        addBlock(std::move(layoutBlock), content);
    }

    cleanupEdges(blocks);

    computeGraphPlacement();

    if (graphCommand != lastShownCommand) {
        selectedBlock = noBlockSelected;
        lastShownCommand = graphCommand;
        center();
    }
}
