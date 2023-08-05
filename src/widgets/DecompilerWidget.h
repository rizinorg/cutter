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
    static QString getWidgetType();
public slots:
    void showDecompilerContextMenu(const QPoint &pt);

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
     * @brief When the synced seek is changed, this refreshes the decompiler widget if needed.
     *
     * Decompiler widget is not refreshed in the following two cases
     *     - Seek changed to an offset contained in the decompiled function.
     *     - Auto-refresh is disabled.
     */
    void seekChanged(RVA /* addr */, CutterCore::SeekHistoryType type);
    void decompilationFinished(RzAnnotatedCode *code);

private:
    std::unique_ptr<Ui::DecompilerWidget> ui;

    RefreshDeferrer *refreshDeferrer;

    bool usingAnnotationBasedHighlighting = false;
    std::unique_ptr<QSyntaxHighlighter> syntaxHighlighter;
    bool decompilerSelectionEnabled;

    /**
     * True if the selected decompiler is currently running a decompilation for this widget. Once
     * the decompilation is over, this should be set to false.
     */
    bool decompilerBusy;

    bool seekFromCursor;
    int historyPos;
    QVector<QPair<int, int>> scrollHistory;
    RVA previousFunctionAddr;
    RVA decompiledFunctionAddr;
    std::unique_ptr<RzAnnotatedCode, void (*)(RzAnnotatedCode *)> code;

    /**
     * Specifies the lowest offset of instructions among all the instructions in the decompiled
     * function.
     */
    RVA lowestOffsetInCode;
    /**
     * Specifies the highest offset of instructions among all the instructions in the decompiled
     * function.
     */
    RVA highestOffsetInCode;

    /**
     * @brief Gets the current decompiler selected by the user.
     *
     * @return A pointer to the currently selected decompiler
     */
    Decompiler *getCurrentDecompiler();

    /**
     * @brief Calls the function doRefresh() if the address specified is a part of the decompiled
     * function.
     *
     * @param addr Address at which a change occurred.
     */
    void refreshIfChanged(RVA addr);
    /**
     * @brief Refreshes the decompiler.
     *
     * - This does the following if the specified offset is valid
     *     - Decompile function that contains the specified offset.
     *     - Clears all selections stored for highlighting purposes.
     *     - Reset previousFunctionAddr with the current function's address
     *       and decompiledFunctionAddr with the address of the function that
     *       was decompiled.
     * - If the offset is invalid, error message is shown in the text widget.
     *
     * @param addr Specified offset/offset in sync.
     */
    void doRefresh();
    /**
     * @brief Update fonts
     */
    void setupFonts();
    /**
     * @brief Update highlights in the text widget.
     *
     * These include respective highlights for:
     *     - Line under cursor
     *     - Word under cursor
     *     - Program Counter(PC) while debugging
     */
    void updateSelection();
    /**
     * @brief Connect/Disconnect SIGNAL-SLOT connection that deals with changes in cursor position.
     *
     * If the argument is true, then connect the SIGNAL-SLOT connection
     * that changes the view as cursor position gets changed in the text widget.
     * Otherwise, disconnect the corresponding signal with slot.
     *
     * @param connectPositionChange
     */
    void connectCursorPositionChanged(bool connectPositionChange);
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
     * @brief This function is responsible for highlighting all the breakpoints in the decompiler
     * view. It will also run when a breakpoint is added, removed or modified.
     */
    void highlightBreakpoints();
    /**
     * @brief Finds the earliest offset and breakpoints within the specified range [startPos,
     * endPos] in the specified RzAnnotatedCode.
     *
     * This function is supposed to be used for finding the earliest offset and breakpoints within
     * the specified range [startPos, endPos]. This will set the value of the variables 'RVA
     * firstOffsetInLine' and 'QVector<RVA> availableBreakpoints' in the context menu.
     *
     * @param codeDecompiled - A reference to the RzAnnotatedCode for the function that is
     * decompiled.
     * @param startPos - Position of the start of the range(inclusive).
     * @param endPos - Position of the end of the range(inclusive).
     */
    void gatherBreakpointInfo(RzAnnotatedCode &codeDecompiled, size_t startPos, size_t endPos);
    /**
     * @brief Finds the global variable reference that's closes to the specified position in the
     * decompiled code. Same as offsetForPosition but for global references only
     *
     * @note If no global reference annotations are found at the given position, an RVA_INVALID is
     * returned
     *
     * @param pos - Position in the decompiled code
     * @return Address of the referenced global for the specified position, or RVA_INVALID if none
     * is found
     */
    ut64 findReference(size_t pos);
    /**
     * @brief Finds the offset that's closest to the specified position in the decompiled code.
     *
     * @note If no annotations that covers the specified position is found, the first offset in the
     * line containing specified position will be returned
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
    void updateBreakpoints(RVA addr);
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
    /**
     * @brief Checks if the specified address is a part of the decompiled function.
     *
     * @param addr An offset in the binary.
     * @return True if the specified is a part of the decompiled function, False otherwise.
     */
    bool addressInRange(RVA addr);

    void setCode(RzAnnotatedCode *code);

    void setHighlighter(bool annotationBasedHighlighter);
};

#endif // DECOMPILERWIDGET_H
