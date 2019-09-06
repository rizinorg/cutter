#ifndef DECOMPILERWIDGET_H
#define DECOMPILERWIDGET_H

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
class DisassemblyContextMenu;
struct DecompiledCodeTextLine;

class DecompilerWidget : public MemoryDockWidget
{
    Q_OBJECT
protected:
    DisassemblyContextMenu *mCtxMenu;

public:
    explicit DecompilerWidget(MainWindow *main, QAction *action = nullptr);
    ~DecompilerWidget();
public slots:
    void showDisasContextMenu(const QPoint &pt);

private slots:
    void fontsUpdated();
    void colorsUpdatedSlot();
    void refreshDecompiler();
    void decompilerSelected();
    void cursorPositionChanged();
    void seekChanged();
    void decompilationFinished(AnnotatedCode code);

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
    AnnotatedCode code;

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
};

#endif // DECOMPILERWIDGET_H
