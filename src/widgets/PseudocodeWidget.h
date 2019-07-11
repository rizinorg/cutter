#ifndef PSEUDOCODEWIDGET_H
#define PSEUDOCODEWIDGET_H

#include <memory>

#include "core/Cutter.h"
#include "MemoryDockWidget.h"

namespace Ui {
class PseudocodeWidget;
}

class QTextEdit;
class QSyntaxHighlighter;
class QTextCursor;
struct DecompiledCodeTextLine;

class PseudocodeWidget : public MemoryDockWidget
{
    Q_OBJECT

public:
    explicit PseudocodeWidget(MainWindow *main, QAction *action = nullptr);
    ~PseudocodeWidget();

private slots:
    void fontsUpdated();
    void colorsUpdatedSlot();
    void refreshPseudocode();
    void cursorPositionChanged();
    void seekChanged();

private:
    enum DecompilerComboBoxValues { DecompilerCBR2Dec, DecompilerCBPdc };
    std::unique_ptr<Ui::PseudocodeWidget> ui;

    QSyntaxHighlighter *syntaxHighlighter;

    /**
     * Index of all lines that are currently displayed, ordered by the position in the text
     */
    QList<DecompiledCodeTextLine> textLines;

    bool seekFromCursor = false;

    void doRefresh(RVA addr);
    void setupFonts();
    void updateSelection();
    void connectCursorPositionChanged(bool disconnect);
    void updateCursorPosition();

    /**
     * @return Iterator to the first line that is after position or last if not found.
     */
    QList<DecompiledCodeTextLine>::iterator findLine(int position);

    /**
     * @return Iterator to the first line that is considered to contain offset
     */
    QList<DecompiledCodeTextLine>::iterator findLineByOffset(RVA offset);
    RVA getOffsetAtLine(const QTextCursor &tc);

    QString getWindowTitle() const override;
};

#endif // PSEUDOCODEWIDGET_H
