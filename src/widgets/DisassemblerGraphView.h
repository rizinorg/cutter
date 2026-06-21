#ifndef DISASSEMBLERGRAPHVIEW_H
#define DISASSEMBLERGRAPHVIEW_H

// Based on the DisassemblerGraphView from x64dbg

#include "common/CutterSeekable.h"
#include "common/RichTextPainter.h"
#include "menus/DisassemblyContextMenu.h"
#include "widgets/CutterGraphView.h"

#include <QLabel>
#include <QPainter>
#include <QShortcut>
#include <QWidget>

class QTextEdit;
class FallbackSyntaxHighlighter;

/**
 * @brief Graph View widget for disassembly
 */
class DisassemblerGraphView : public CutterGraphView
{
    Q_OBJECT

    struct Text
    {
        std::vector<RichTextPainter::List> lines;

        Text() {}

        Text(const QString &text, QColor color, QColor background)
        {
            RichTextPainter::List richText;
            RichTextPainter::CustomRichText rt;
            rt.highlight = false;
            rt.text = text;
            rt.textColor = color;
            rt.textBackground = background;
            rt.flags = rt.textBackground.alpha() ? RichTextPainter::FlagAll
                                                 : RichTextPainter::FlagColor;
            richText.push_back(rt);
            lines.push_back(richText);
        }

        Text(const RichTextPainter::List &richText) { lines.push_back(richText); }

        QString toQString() const
        {
            QString result;
            for (const auto &line : lines) {
                for (const auto &t : line) {
                    result += t.text;
                }
            }
            return result;
        }
    };

    struct Instr
    {
        ut64 addr = 0;
        ut64 size = 0;
        Text text;
        Text fullText;
        QString plainText;
        std::vector<unsigned char> opcode; // instruction bytes

        bool empty() const { return size == 0; }
        bool contains(ut64 addr) const;
    };

    struct Token
    {
        int start;
        int length;
        QString type;
        Instr *instr;
        QString name;
        QString content;
    };

    struct DisassemblyBlock
    {
        Text headerText;
        std::vector<Instr> instrs;
        ut64 entry = 0;
        ut64 truePath = 0;
        ut64 falsePath = 0;
        bool terminal = false;
        bool indirectcall = false;
    };

public:
    DisassemblerGraphView(QWidget *parent, CutterSeekable *seekable, MainWindow *mainWindow,
                          const QList<QAction *> &additionalMenuAction);
    ~DisassemblerGraphView() override;
    std::unordered_map<ut64, DisassemblyBlock> disassemblyBlocks;
    virtual void drawBlock(QPainter &p, GraphView::GraphBlock &block, bool interactive) override;
    virtual void blockClicked(GraphView::GraphBlock &block, QMouseEvent *event,
                              QPoint pos) override;
    virtual void blockDoubleClicked(GraphView::GraphBlock &block, QMouseEvent *event,
                                    QPoint pos) override;
    virtual bool helpEvent(QHelpEvent *event) override;
    virtual void blockHelpEvent(GraphView::GraphBlock &block, QHelpEvent *event,
                                QPoint pos) override;
    virtual GraphView::EdgeConfiguration edgeConfiguration(GraphView::GraphBlock &from,
                                                           GraphView::GraphBlock *to,
                                                           bool interactive) override;
    virtual void blockTransitionedTo(GraphView::GraphBlock *to) override;

    void loadCurrentGraph();
    QString windowTitle;

    int getWidth() { return width; }
    int getHeight() { return height; }
    std::unordered_map<ut64, GraphBlock> getBlocks() { return blocks; }
    using EdgeConfigurationMapping = std::map<std::pair<ut64, ut64>, EdgeConfiguration>;
    EdgeConfigurationMapping getEdgeConfigurations();

    /**
     * @brief keep the current addr of the fcn of Graph
     * Everytime overview updates its contents, it compares this value with the one in Graph
     * if they aren't same, then Overview needs to update the pixmap cache.
     */
    ut64 currentFcnAddr = RVA_INVALID; // TODO: make this less public
public slots:
    void refreshView() override;

    void onSeekChanged(RVA addr);
    void takeTrue();
    void takeFalse();

    void nextInstr();
    void prevInstr();

    void copySelection();

protected:
    void paintEvent(QPaintEvent *event) override;
    void blockContextMenuRequested(GraphView::GraphBlock &block, QContextMenuEvent *event,
                                   QPoint pos) override;
    void contextMenuEvent(QContextMenuEvent *event) override;
    void restoreCurrentBlock() override;
    bool eventFilter(QObject *obj, QEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;

private slots:
    void showExportDialog() override;
    void onActionHighlightBITriggered();
    void onActionUnhighlightBITriggered();
    void setTooltipStylesheet();

private:
    bool transitionDontSeek = false;

    Token *highlightToken;
    bool emptyGraph;
    ut64 currentBlockAddress = RVA_INVALID;

    DisassemblyContextMenu *blockMenu;
    QMenu *contextMenu;

    void connectSeekChanged(bool disconnect);

    void prepareGraphNode(GraphBlock &block);
    Token *getToken(Instr *instr, int x);

    QPoint getInstructionOffset(const DisassemblyBlock &block, int line) const;
    RVA getAddrForMouseEvent(GraphBlock &block, QPoint *point);
    Instr *getInstrForMouseEvent(GraphBlock &block, QPoint *point, bool force = false);
    /**
     * @brief Get instructions placement and size relative to block.
     * Inefficient don't use this function when iterating over all instructions.
     * @param block
     * @param addr
     * @return
     */
    QRectF getInstrRect(GraphView::GraphBlock &block, RVA addr) const;
    /**
     * @brief Get the true path address if the offset represents the last instruction of block
     * @param offset Offset to check
     * @return RVA Offset for true path
     */
    RVA getTruePathForOffset(RVA offset);
    void showInstruction(GraphView::GraphBlock &block, RVA addr);
    const Instr *instrForAddress(RVA addr);
    DisassemblyBlock *blockForAddress(RVA addr);
    void seekLocal(RVA addr, bool update_viewport = true);
    void seekInstruction(bool previous_instr);

    CutterSeekable *seekable = nullptr;
    QList<QShortcut *> shortcuts;
    QList<RVA> breakpoints;

    QAction actionUnhighlight;
    QAction actionUnhighlightInstruction;

    QLabel *emptyText = nullptr;

signals:
    void nameChanged(const QString &name);

public:
    bool isGraphEmpty() const { return emptyGraph; }
};

#endif // DISASSEMBLERGRAPHVIEW_H
