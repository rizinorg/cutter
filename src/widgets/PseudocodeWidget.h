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

    QSyntaxHighlighter *syntaxHighlighter;
    bool decompilerSelectionEnabled;

    AnnotatedCode code;

    bool seekFromCursor = false;

    void doRefresh(RVA addr);
    void setupFonts();
    void updateSelection();
    void connectCursorPositionChanged(bool disconnect);
    void updateCursorPosition();

    QString getWindowTitle() const override;
};

#endif // PSEUDOCODEWIDGET_H
