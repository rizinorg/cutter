#ifndef DECOMPILERWIDGET_H
#define DECOMPILERWIDGET_H

#include <QTextEdit>
#include <memory>

#include "core/Cutter.h"
#include "MemoryDockWidget.h"
#include "Decompiler.h"

namespace Ui {
class DecompilerWidget;
}

class QTextEdit;
class QSyntaxHighlighter;
class QTextCursor;
class DecompilerContextMenu;
struct DecompiledCodeTextLine;

class DecompilerWidget : public MemoryDockWidget
{
    Q_OBJECT
protected:
    DecompilerContextMenu *mCtxMenu;

public:
    explicit DecompilerWidget(MainWindow *main);
    ~DecompilerWidget();
public slots:
    void showDisasContextMenu(const QPoint &pt);

    void highlightPC();
private slots:
    /**
     * @brief Copy to clipboard what's needed depending on the state of text widget.
     * 
     * @note If something is selected in the text widget, copy selection.
     *       If something is highlighted, copy highlighted word.
     *       Otherwise, copy the line under cursor.
     */
    void copy();
    void fontsUpdatedSlot();
    void colorsUpdatedSlot();
    void refreshDecompiler();
    void decompilerSelected();
    void cursorPositionChanged();
    /**
     * @brief When the synced seek is changed, this refreshes the decompiler
     * if needed
     */
    void seekChanged();
    void decompilationFinished(RAnnotatedCode *code);

private:
    std::unique_ptr<Ui::DecompilerWidget> ui;

    RefreshDeferrer *refreshDeferrer;

    QSyntaxHighlighter *syntaxHighlighter;
    bool decompilerSelectionEnabled;
    bool autoRefreshEnabled;

    /**
     * True if doRefresh() was called, but the decompiler was still running
     * This means, after the decompiler has finished, it should be refreshed immediately.
     */
    bool decompilerWasBusy;

    int scrollerHorizontal;
    int scrollerVertical;
    RVA previousFunctionAddr;
    RVA decompiledFunctionAddr;
    std::unique_ptr<RAnnotatedCode, void (*)(RAnnotatedCode *)> code;
    bool seekFromCursor = false;

    Decompiler *getCurrentDecompiler();

    /**
     * @brief Enable/Disable auto refresh as per the specified boolean value
     * 
     * @param enabled
     */
    void setAutoRefresh(bool enabled);
    /**
     * @brief Do refersh if the auto refresh is enabled
     */
    void doAutoRefresh();
    /**
     * @brief Decompile the function that contains the specified offset.
     * 
     * @param addr Specified offset/offset in sync.
     */
    void doRefresh(RVA addr = Core()->getOffset());
    void updateRefreshButton();
    /**
     * @brief Update fonts
     */
    void setupFonts();
    /**
     * @brief Update highlights in the text widget
     */
    void updateSelection();
    /**
     * @brief Connect/Disconnect SIGNAL-SLOT connection that deals with changes in cursor position.
     * 
     * If the argument is true, then disconnect the SIGNAL-SLOT connection
     * that changes the view as cursor position gets changed in the text widget.
     * Otherwise, connect the corresponding signal with slot.
     * 
     * @param disconnect
     */
    void connectCursorPositionChanged(bool disconnect);
    /**
     * @brief Find the current global offset in sync and update cursor
     * to the position specified by this offset (found using positionForOffset() )
     */
    void updateCursorPosition();

    QString getWindowTitle() const override;

    /**
     * @brief Event filter that intercept the following events:
     *     1. Double click
     *     2. Right click
     * 
     * @param obj
     * @param event
     * @return 
     */
    bool eventFilter(QObject *obj, QEvent *event) override;

    /**
     * @brief a wrapper around CutterSeekable::seekToReference to seek to an object which is
     * referenced from the address under cursor
     */
    void seekToReference();

    /**
     * @brief Retrieve the Cursor for a location as close as possible to the given address
     * @param addr - an address in the decompiled function
     * @return a Cursor object for the given address
     */
    QTextCursor getCursorForAddress(RVA addr);

    /**
     * @brief Append a highlighted line to the TextEdit
     * @param extraSelection - an ExtraSelection object colored with the appropriate color
     * @return True on success, otherwise False
     */
    bool colorLine(QTextEdit::ExtraSelection extraSelection);

    /**
     * @brief This function responsible to highlight all the breakpoints in the decompiler view.
     * It will also run when a breakpoint is added, removed or modified.
     */
    void highlightBreakpoints();
    /**
     * @brief Finds the earliest offset and breakpoints within the specified range [startPos, endPos]
     * in the specified RAnnotatedCode.
     *
     * This function is supposed to be used for finding the earliest offset and breakpoints within the specified range
     * [startPos, endPos]. This will set the value of the variables 'RVA firstOffsetInLine' and 'QVector<RVA> availableBreakpoints' in
     * the context menu.
     *
     * @param codeDecompiled - A reference to the RAnnotatedCode for the function that is decompiled.
     * @param startPos - Position of the start of the range(inclusive).
     * @param endPos - Position of the end of the range(inclusive).
     */
    void gatherBreakpointInfo(RAnnotatedCode &codeDecompiled, size_t startPos, size_t endPos);
    /**
     * @brief Finds the offset that's closest to the specified position in the decompiled code.
     * 
     * @note If no annotations that covers the specified position is found, the first offset in the line
     * containing specified position will be returned
     * 
     * @param pos - Position of the decompiled code.
     * @return Offset for the specified position/first offset in line.
     */
    ut64 offsetForPosition(size_t pos);
    /**
     * @brief Find the start position of the annotation with the offset that's closest to
     * the specified offset
     * 
     * @param offset
     * @return Position found or SIZE_MAX
     */
    size_t positionForOffset(ut64 offset);
    /**
     * @brief Updates the view when breakpoints are changed
     */
    void updateBreakpoints();
    /**
     * @brief Set information about the breakpoints on the line in the context menu
     */
    void setInfoForBreakpoints();
    /**
     * @brief Find the context-related annotation covering the specified position. 
     * If found, set the variable annotationHere in the decompiler context menu.
     * 
     * @param pos Position of cursor in the decompiled code.
     */
    void setAnnotationsAtCursor(size_t pos);
};

#endif // DECOMPILERWIDGET_H
