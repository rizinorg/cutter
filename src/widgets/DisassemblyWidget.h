#ifndef DISASSEMBLYWIDGET_H
#define DISASSEMBLYWIDGET_H

#include "core/Cutter.h"
#include "MemoryDockWidget.h"
#include "common/CutterSeekable.h"
#include "common/RefreshDeferrer.h"

#include <QTextEdit>
#include <QPlainTextEdit>
#include <QShortcut>
#include <QAction>


class DisassemblyTextEdit;
class DisassemblyScrollArea;
class DisassemblyContextMenu;

class DisassemblyWidget : public MemoryDockWidget
{
    Q_OBJECT
public:
    explicit DisassemblyWidget(MainWindow *main, QAction *action = nullptr);
    QWidget *getTextWidget();

public slots:
    void highlightCurrentLine();
    void showDisasContextMenu(const QPoint &pt);
    void fontsUpdatedSlot();
    void colorsUpdatedSlot();
    void seekPrev();
    void toggleSync();
    void setPreviewMode(bool previewMode);

protected slots:
    void on_seekChanged(RVA offset);
    void refreshDisasm(RVA offset = RVA_INVALID);

    void scrollInstructions(int count);
    bool updateMaxLines();

    void cursorPositionChanged();

    void zoomIn();
    void zoomOut();

protected:
    DisassemblyContextMenu *mCtxMenu;
    DisassemblyScrollArea *mDisasScrollArea;
    DisassemblyTextEdit *mDisasTextEdit;

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
    RVA readDisassemblyOffset(QTextCursor tc);
    bool eventFilter(QObject *obj, QEvent *event) override;

    QList<RVA> breakpoints;

    void setupFonts();
    void setupColors();

    void updateCursorPosition();

    void connectCursorPositionChanged(bool disconnect);

    void moveCursorRelative(bool up, bool page);
    QList<QTextEdit::ExtraSelection> getSameWordsSelections();

    QAction syncIt;
    CutterSeekable *seekable;
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
    explicit DisassemblyTextEdit(QWidget *parent = nullptr)
        : QPlainTextEdit(parent),
          lockScroll(false) {}

    void setLockScroll(bool lock)
    {
        this->lockScroll = lock;
    }

protected:
    bool viewportEvent(QEvent *event) override;
    void scrollContentsBy(int dx, int dy) override;
    void keyPressEvent(QKeyEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;

private:
    bool lockScroll;
};

#endif // DISASSEMBLYWIDGET_H
