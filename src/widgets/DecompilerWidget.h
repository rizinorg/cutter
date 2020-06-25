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
    void fontsUpdatedSlot();
    void colorsUpdatedSlot();
    void refreshDecompiler();
    void decompilerSelected();
    void cursorPositionChanged();
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

    RVA decompiledFunctionAddr;
    std::unique_ptr<RAnnotatedCode, void (*)(RAnnotatedCode *)> code;
    bool seekFromCursor = false;

    Decompiler *getCurrentDecompiler();

    void setAutoRefresh(bool enabled);
    void doAutoRefresh();
    void doRefresh(RVA addr = Core()->getOffset());
    void updateRefreshButton();
    void setupFonts();
    void updateSelection();
    void connectCursorPositionChanged(bool disconnect);
    void updateCursorPosition();

    QString getWindowTitle() const override;
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
     * @brief This function is responsible for setting details regarding the breakpoints for
     * every single line of decompiled code. This will set variable firstOffsetInLine and
     * QVector<RVA> availableBreakpoints for the line that a cursor points to.
     *
     * @param codeDecompiled - A reference to the RAnnotatedCode for the function that is decompiled.
     * @param startPos - Position of the character from where the corresponding line starts in the decompiled code.
     * @param endPos - Position of the character at where the corresponding line ends in the decompiled code.
     */
    void setBreakpointInfo(RAnnotatedCode &codeDecompiled, size_t startPos, size_t endPos);

};

#endif // DECOMPILERWIDGET_H
