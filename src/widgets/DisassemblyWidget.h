#ifndef DISASSEMBLYWIDGET_H
#define DISASSEMBLYWIDGET_H

#include "core/Cutter.h"
#include "MemoryDockWidget.h"
#include "common/CutterSeekable.h"
#include "common/RefreshDeferrer.h"
#include "common/CachedFontMetrics.h"

#include <QTextEdit>
#include <QPlainTextEdit>
#include <QShortcut>
#include <QAction>

#include <vector>

class DisassemblyTextEdit;
class DisassemblyScrollArea;
class DisassemblyContextMenu;
class DisassemblyLeftPanel;
class AddressRangeScrollBar;

class DisassemblyWidget : public MemoryDockWidget
{
    Q_OBJECT
public:
    explicit DisassemblyWidget(MainWindow *main);
    QWidget *getTextWidget();

    static QString getWidgetType();

public slots:
    /**
     * @brief Highlights the currently selected line and updates the
     * highlighting of the same words under the cursor in the visible screen.
     * This overrides all previous highlighting.
     */
    void highlightCurrentLine();
    /**
     * @brief Adds the PC line highlighting to the other current highlighting.
     * This should be called after highlightCurrentLine since that function
     * overrides all previous highlighting.
     */
    void highlightPCLine();
    void showDisasContextMenu(const QPoint &pt);
    void fontsUpdatedSlot();
    void colorsUpdatedSlot();
    void scrollInstructions(int count, bool clampToScrollBarRange = false);
    void seekPrev();
    void setPreviewMode(bool previewMode);
    QFontMetricsF getFontMetrics();
    QList<DisassemblyLine> getLines();

    /**
     * @brief Reposts a wheel event to vertical scrollbar
     *
     * @param event The original QWheelEvent to be processed by the scrollbar
     */
    void repostWheelEvent(QWheelEvent *event);
protected slots:
    void on_seekChanged(RVA offset, CutterCore::SeekHistoryType type);
    void refreshIfInRange(RVA offset);
    void instructionChanged(RVA offset);
    void refreshDisasm(RVA offset = RVA_INVALID);

    bool updateMaxLines();

    void cursorPositionChanged();

protected:
    DisassemblyContextMenu *mCtxMenu;
    DisassemblyScrollArea *mDisasScrollArea;
    DisassemblyTextEdit *mDisasTextEdit;
    DisassemblyLeftPanel *leftPanel;
    QList<DisassemblyLine> lines;

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

    QList<RVA> breakpoints;

    void setupFonts();
    void setupColors();

    void updateCursorPosition();

    void connectCursorPositionChanged(bool disconnect);

    void moveCursorRelative(bool up, bool page);

    void jumpToOffsetUnderCursor(const QTextCursor &);
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
    void wheelEventTriggered(QWheelEvent *event);

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
    explicit DisassemblyTextEdit(QWidget *parent = nullptr)
        : QPlainTextEdit(parent), lockScroll(false)
    {
    }

    void setLockScroll(bool lock) { this->lockScroll = lock; }

    qreal textOffset() const;

protected:
    bool viewportEvent(QEvent *event) override;
    void scrollContentsBy(int dx, int dy) override;
    void keyPressEvent(QKeyEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;

private:
    bool lockScroll;
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
