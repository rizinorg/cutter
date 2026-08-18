#ifndef DIFFGRAPHVIEW_H
#define DIFFGRAPHVIEW_H

#include <QWidget>

#include <Configuration.h>
#include <CutterDiff.h>
#include <widgets/CutterGraphView.h>
class QTextEdit;

enum DiffGraphMode : ut8 { Unified, Original, Modified };

class DiffGraphView : public CutterGraphView
{
    Q_OBJECT

    struct DiffBlock
    {
        QString headerText;
        ut64 offset = 0;
        ut64 entry = 0;
        ut64 truePathA = 0;
        ut64 falsePathA = 0;
        ut64 truePathB = 0;
        ut64 falsePathB = 0;
        QHash<qulonglong, DiffItemType> caseOps;
        bool terminal = false;
        bool indirectcall = false;
        DiffItemType type;
        QList<DiffInstr> instrs;
    };

public:
    explicit DiffGraphView(CutterDiff *cutterDiff, QWidget *parent = nullptr);
    ~DiffGraphView() override;
    std::unordered_map<ut64, DiffBlock> diffBlocks;
    virtual void drawBlock(QPainter &p, GraphView::GraphBlock &block, bool interactive) override;
    virtual void blockClicked(GraphView::GraphBlock &block, QMouseEvent *event,
                              QPoint pos) override;
    virtual void blockDoubleClicked(GraphView::GraphBlock &block, QMouseEvent *event,
                                    QPoint pos) override;
    virtual GraphView::EdgeConfiguration edgeConfiguration(GraphView::GraphBlock &from,
                                                           GraphView::GraphBlock *to,
                                                           bool interactive) override;
    void loadCurrentGraph(DiffGraphMode graphMode);
    int getWidth() { return width; }
    int getHeight() { return height; }
    std::unordered_map<ut64, GraphBlock> getBlocks() { return blocks; }
    using EdgeConfigurationMapping = std::map<std::pair<ut64, ut64>, EdgeConfiguration>;
    EdgeConfigurationMapping getEdgeConfigurations();
    ut64 currentFcnAddr = RVA_INVALID; // TODO: make this less public
    void addDiffGraphBlockMatched(const CutterDiffItem &diffItem);
    void addDiffGraphBlockMismatch(const CutterDiffItem &diffItem);
    void drawDiffLine(QPainter &pen, const QString &instr, int x, int y,
                      DiffInstrType type = DiffInstrEqual);
    ut64 graphEntryFromOffset(ut64 offset, bool original);
public slots:
    void refreshView() override;
    void copySelection() {}

protected:
    void paintEvent(QPaintEvent *event) override;
    void blockContextMenuRequested(GraphView::GraphBlock &block, QContextMenuEvent *event,
                                   QPoint pos) override;
    void contextMenuEvent(QContextMenuEvent *event) override;
    void restoreCurrentBlock() override {}
    bool eventFilter(QObject *obj, QEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;
private slots:
    void showExportDialog() override;
    void setTooltipStylesheet();

private:
    bool emptyGraph;
    ut64 currentBlockAddress = RVA_INVALID;
    QMenu *contextMenu;

    void prepareGraphNode(GraphBlock &block);
    DiffBlock *blockForAddress(RVA addr);
    QLabel *emptyText = nullptr;
    CutterDiff *cutterDiff;
    ut64 currentBlockEntry = 0;
    // offsetMapper

    std::unordered_map<ut64, ut64> blocksA;
    std::unordered_map<ut64, ut64> blocksB;
signals:
    void nameChanged(const QString &name);

private:
    DiffGraphMode diffGraphMode = Unified;
};

#endif // DIFFGRAPHVIEW_H
