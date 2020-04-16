#ifndef MEMORYDOCKWIDGET_H
#define MEMORYDOCKWIDGET_H

#include "CutterDockWidget.h"
#include "core/Cutter.h"

#include <QAction>

class CutterSeekable;

/* Disassembly/Graph/Hexdump/Decompiler view priority */
enum class MemoryWidgetType { Disassembly, Graph, Hexdump, Decompiler };

class MemoryDockWidget : public CutterDockWidget
{
    Q_OBJECT
public:
    MemoryDockWidget(MemoryWidgetType type, MainWindow *parent, QAction *action = nullptr);
    ~MemoryDockWidget() override {}

    CutterSeekable *getSeekable() const;

    bool tryRaiseMemoryWidget();
    void raiseMemoryWidget();
    MemoryWidgetType getType() const
    {
        return mType;
    }
    bool eventFilter(QObject *object, QEvent *event) override;
private:

    MemoryWidgetType mType;

public slots:
    void updateWindowTitle();

protected:
    CutterSeekable *seekable = nullptr;
    QAction syncAction;
    QMenu *dockMenu = nullptr;

    virtual QString getWindowTitle() const = 0;
    void contextMenuEvent(QContextMenuEvent *event) override;
};

#endif // MEMORYDOCKWIDGET_H
