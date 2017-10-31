#ifndef DISASSEMBLYWIDGET_H
#define DISASSEMBLYWIDGET_H

#include "cutter.h"
#include <QDockWidget>
#include <QPlainTextEdit>
#include <QShortcut>


class DisassemblyTextEdit;
class DisassemblyScrollArea;

class DisassemblyWidget : public QDockWidget
{
    Q_OBJECT
public:
    explicit DisassemblyWidget(QWidget *parent = nullptr);
    explicit DisassemblyWidget(const QString &title, QWidget *parent = nullptr);
    QWidget* getTextWidget();

public slots:
    void highlightCurrentLine();
    void showDisasContextMenu(const QPoint &pt);
    void cursorPositionChanged();
    void on_seekChanged(RVA offset);
    void refreshDisasm(RVA offset = RVA_INVALID);
    void fontsUpdatedSlot();
    void showXrefsDialog();

private slots:
    void scrollInstructions(int count);
    void updateMaxLines();

private:
    DisassemblyScrollArea *mDisasScrollArea;
    DisassemblyTextEdit *mDisasTextEdit;

    RVA topOffset;
    int maxLines;

    QString readDisasm(RVA offset, bool backwards = false, bool skipFirstInstruction = false);
    QString readDisasm(const QString &cmd, bool stripLastNewline);
    RVA readCurrentDisassemblyOffset();
    void highlightDisasms();
    bool eventFilter(QObject *obj, QEvent *event);
};

class DisassemblyScrollArea : public QAbstractScrollArea
{
    Q_OBJECT

public:
    explicit DisassemblyScrollArea(QWidget *parent = nullptr);

signals:
    void scrollLines(int lines);
    void disassemblyResized();

protected:
    bool viewportEvent(QEvent *event) override;

private:
    void resetScrollBars();
};


class DisassemblyTextEdit: public QPlainTextEdit
{
    Q_OBJECT

public:
    explicit DisassemblyTextEdit(QWidget *parent = nullptr) : QPlainTextEdit(parent) {}

protected:
    bool viewportEvent(QEvent *event) override;
};

#endif // DISASSEMBLYWIDGET_H
