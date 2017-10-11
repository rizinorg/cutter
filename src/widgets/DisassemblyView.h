#ifndef DISASSEMBLYVIEW_H
#define DISASSEMBLYVIEW_H

#include <QDockWidget>
#include <QTextEdit>
#include "cutter.h"

class DisassemblyView : public QDockWidget
{
    Q_OBJECT
public:
    explicit DisassemblyView(QWidget *parent = nullptr);
    explicit DisassemblyView(const QString &title, QWidget *parent = nullptr);

signals:

public slots:
    void highlightCurrentLine();
    void disasmScrolled();
    void showDisasContextMenu(const QPoint &pt);
    void showXrefsDialog();
    void on_mDisasTextEdit_cursorPositionChanged();
    void on_seekChanged(RVA offset);
    void refreshDisasm();

private:
    QTextEdit *mDisasTextEdit;

    RVA readCurrentDisassemblyOffset();
    bool loadMoreDisassembly();
    void highlightDisasms();
    bool eventFilter(QObject *obj, QEvent *event);
};

#endif // DISASSEMBLYVIEW_H
