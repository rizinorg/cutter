#ifndef DISASSEMBLYWIDGET_H
#define DISASSEMBLYWIDGET_H

#include "MemoryDockWidget.h"
#include "common/CutterSeekable.h"
#include "common/RefreshDeferrer.h"
#include "core/Cutter.h"

#include <QAction>
#include <QPlainTextEdit>
#include <QShortcut>
#include <QTextEdit>

#include <vector>

class DisassemblyTextEdit;
class DisassemblyScrollArea;
class DisassemblyContextMenu;
class DisassemblyLeftPanel;
class AddressRangeScrollBar;

enum class RefreshMode : ut8 { Append, Prepend, Reset, Keep, None };

/**
 * @brief Main widget for showing disassembly of a binary
 *
 * @see DisassemblerGraphView
 */
class DisassemblyWidget : public MemoryDockWidget
{
    Q_OBJECT
public:
    explicit DisassemblyWidget(MainWindow *main);
    QWidget *getTextWidget();

    static QString getWidgetType();

    QFontMetricsF getFontMetrics();
    QList<DisassemblyLine> getLines();

    int getStartIndex() const;
    int getEndIndex() const;

    /**
     * @brief Updates the offset and character position where the cursor selection ends
     */
    void updateSelectionPos(const QTextCursor &cursor);

    /**
     * @brief Updates the offset and character position where the cursor selection starts
     */
    void updateSelectionAnchor(const QTextCursor &cursor);

public slots:
    /**
     * @brief Highlights the currently selected line and updates the
     * highlighting of the same words under the cursor in the visible screen.
     * @return List of selections to be highlighted
     */
    QList<QTextEdit::ExtraSelection> highlightCurrentLine();
    /**
     * @brief Adds the PC line highlighting to the other current highlighting.
     * This is generally called after highlightCurrentLine
     * @return List of selections to be highlighted
     */
    QList<QTextEdit::ExtraSelection> highlightPCLine();
    void showDisasContextMenu(const QPoint &pt);
    void fontsUpdatedSlot();
    void colorsUpdatedSlot();
    void scrollInstructions(int count, bool clampToScrollBarRange = false);
    void seekPrev();
    void setPreviewMode(bool previewMode);

    /**
     * @brief Forces the transient vertical scrollbar to appear on scroll
     */
    void showTransientScrollBar();

    void refreshDisasm(RVA offset = RVA_INVALID, RefreshMode mode = RefreshMode::Reset);

protected slots:
    void onSeekChanged(RVA offset, CutterCore::SeekHistoryType type);
    void refreshIfInRange(RVA offset);
    void instructionChanged(RVA offset);

    bool updateMaxLines();

    void cursorPositionChanged();
    /**
     * @brief Copies the currently highlighted disassembly text to the system clipboard
     */
    void copySelection();

protected:
    DisassemblyContextMenu *mCtxMenu;
    DisassemblyScrollArea *mDisasScrollArea;
    DisassemblyTextEdit *mDisasTextEdit;
    DisassemblyLeftPanel *leftPanel;

private:
    RVA topOffset;
    RVA bottomOffset;
    int maxLines;

    QString curHighlightedWord;

    /**
     * offset of lines below the first line of the current seek
     */
    int cursorLineOffset;
    int cursorCharOffset;
    bool seekFromCursor;

    RefreshDeferrer *disasmRefresh;

    RVA readCurrentDisassemblyOffset();
    bool eventFilter(QObject *obj, QEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;
    void contextMenuEvent(QContextMenuEvent *event) override;
    QString getWindowTitle() const override;

    int topOffsetHistoryPos = 0;
    QList<RVA> topOffsetHistory;

    int startIndex = 0;
    int endIndex = 0;
    QList<DisassemblyLine> lines;

    // Cursor selection related
    RVA selectionAnchorRVA = RVA_INVALID;
    /**
     * @brief metadata lines attached to instruction have the same offset as the instruction itself,
     * this keeps track of which metdata line the cursor was at inside the offset block
     */
    int selectionAnchorSubIndex = 0;
    int selectionAnchorChar = 0;
    RVA selectionPosRVA = RVA_INVALID;
    /**
     * @brief same use-case as @ref selectionAnchorSubIndex but for cursor position instead if
     * anchor
     */
    int selectionPosSubIndex = 0;
    int selectionPosChar = 0;

    QList<RVA> breakpoints;
    /**
     * @brief Set whenever breakpoints have been updated or the screen has been manually refreshed
     */
    bool breakpointsDirty = true;

    void setupFonts();
    void setupColors();

    void updateCursorPosition();

    void connectCursorPositionChanged(bool disconnect);

    void moveCursorRelative(QTextCursor::MoveOperation op, bool page);

    void jumpToOffsetUnderCursor(const QTextCursor &);

    /**
     * @brief Visually highlights the text on screen between the selection start (anchor) and the
     * current cursor position
     */
    void updateSelection();
    void updateContextMenuSelection(bool hasSelection);

    /**
     * @brief Finds the visual line number in the current view for a specific offset
     * @param offset The memory address (RVA) to look for
     * @param offsetSubIndex The specific sub-line to target if the instructions has metadata
     * attached to it
     * @return The index of the line, or -1 if not found
     *
     * @see DisassemblyHelper::getIndexInOffsetGroup()
     */
    int getLineIndex(RVA offset, int offsetSubIndex) const;
    /**
     * @brief Refreshes the background colors in the disassembly view.
     * It applies highlights to both the line currently under the user's cursor and the Program
     * Counter (PC) line.
     */
    void updateLineHighlights();
    /**
     * @brief Clears the current text selection so nothing is highlighted as selected
     */
    void invalidateCursorSelection();

    /**
     * @brief Metadata lines attached to an instruction have the same offset saved with it as the
     * instruction, This function returns the index of the current line within that offset
     * group/block
     *      * e.g:
     *      * @code
     * ; a comment
     * ; another comment     <------ assume cursor is here
     * ; void func1()
     * 0x1000 mov rax, rax
     * @endcode
     *
     * Then the returned index is "1"
     */
    int getIndexInOffsetGroup(const QTextCursor &cursor) const;
};

class DisassemblyScrollArea : public QAbstractScrollArea
{
    Q_OBJECT

public:
    explicit DisassemblyScrollArea(QWidget *parent = nullptr);
    AddressRangeScrollBar *verticalScrollBar();

signals:
    void scrollLines(int lines, bool clampToScrollBarRange = false);
    void disassemblyResized();
    void wheelEventTriggered();

protected:
    bool viewportEvent(QEvent *event) override;
    void wheelEvent(QWheelEvent *event) override;

private:
    AddressRangeScrollBar *vScrollBar;
    int accumScrollWheelDeltaY;
};

class DisassemblyTextEdit : public QPlainTextEdit
{
    Q_OBJECT

public:
    explicit DisassemblyTextEdit(DisassemblyWidget *disasmWidget = nullptr);

    void setLockScroll(bool lock) { this->lockScroll = lock; }

    qreal textOffset() const;

    void setCursorVisible(bool visible);

protected:
    bool viewportEvent(QEvent *event) override;
    void scrollContentsBy(int dx, int dy) override;
    void keyPressEvent(QKeyEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void paintEvent(QPaintEvent *event) override;

    void mouseMoveEvent(QMouseEvent *event) override;

private:
    bool lockScroll;
    QTimer *blinkTimer;
    bool cursorVisible = true;
    QColor cursorColor;

    DisassemblyWidget *disasmWidget = nullptr;
};

/**
 * This class is used to draw the left pane of the disassembly
 * widget. Its goal is to draw proper arrows for the jumps of the disassembly.
 */
class DisassemblyLeftPanel : public QFrame
{
public:
    DisassemblyLeftPanel(DisassemblyWidget *disas);
    void paintEvent(QPaintEvent *event) override;
    void wheelEvent(QWheelEvent *event) override;
    void clearArrowFrom(RVA offset);

private:
    DisassemblyWidget *disas;

    struct Arrow
    {
        Arrow(RVA v1, RVA v2) : min(v1), max(v2), level(0), up(false)
        {
            if (min > max) {
                std::swap(min, max);
                up = true;
            }
        }

        inline bool contains(RVA point) const { return min <= point && max >= point; }

        inline bool intersects(const Arrow &other) const
        {
            return std::max(min, other.min) <= std::min(max, other.max);
        }

        ut64 length() const { return max - min; }

        RVA jmpFromOffset() const { return up ? max : min; }

        RVA jmpToffset() const { return up ? min : max; }

        RVA min;
        RVA max;
        uint32_t level;
        bool up;
    };

    const size_t arrowsSize = 128;
    const uint32_t maxLevelBeforeFlush = 32;
    RVA lastBeginOffset = 0;
    std::vector<Arrow> arrows;
};

#endif // DISASSEMBLYWIDGET_H
