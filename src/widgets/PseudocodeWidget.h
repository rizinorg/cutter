#ifndef PSEUDOCODEWIDGET_H
#define PSEUDOCODEWIDGET_H

#include <memory>

#include "core/Cutter.h"
#include "MemoryDockWidget.h"
#include "Decompiler.h"

namespace Ui {
class PseudocodeWidget;
}

class QTextEdit;
class QSyntaxHighlighter;
class QTextCursor;
class DisassemblyContextMenu;
struct DecompiledCodeTextLine;

class PseudocodeWidget : public MemoryDockWidget
{
    Q_OBJECT
protected:
    DisassemblyContextMenu *mCtxMenu;

public:
    explicit PseudocodeWidget(MainWindow *main, QAction *action = nullptr);
    ~PseudocodeWidget();
public slots:
    void showDisasContextMenu(const QPoint &pt);

private slots:
    void fontsUpdated();
    void colorsUpdatedSlot();
    void refreshPseudocode();
    void decompilerSelected();
    void cursorPositionChanged();
    void seekChanged();
    void decompilationFinished(AnnotatedCode code);

private:
    std::unique_ptr<Ui::PseudocodeWidget> ui;

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

#endif // PSEUDOCODEWIDGET_H
