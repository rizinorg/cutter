#ifndef MEMORYDOCKWIDGET_H
#define MEMORYDOCKWIDGET_H

#include "CutterDockWidget.h"
#include "core/Cutter.h"

class CutterSeekable;

/* Disassembly/Graph/Hexdump/Decompiler view priority */
enum class MemoryWidgetType { Disassembly, Graph, Hexdump, Decompiler };

class MemoryDockWidget : public CutterDockWidget
{
    Q_OBJECT
public:
    MemoryDockWidget(MemoryWidgetType type, MainWindow *parent, QAction *action = nullptr);
    ~MemoryDockWidget() {}

    CutterSeekable* getSeekable() const;

    bool tryRaiseMemoryWidget();
    void raiseMemoryWidget();
    MemoryWidgetType getType() const
    {
        return mType;
    }
    bool eventFilter(QObject *object, QEvent *event);
private:

    MemoryWidgetType mType;

public slots:
    void updateWindowTitle();

protected:
    CutterSeekable *seekable = nullptr;

    virtual QString getWindowTitle() const = 0;
};

#endif // MEMORYDOCKWIDGET_H
