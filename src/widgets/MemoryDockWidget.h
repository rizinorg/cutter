#ifndef MEMORYDOCKWIDGET_H
#define MEMORYDOCKWIDGET_H

#include "CutterDockWidget.h"
#include "core/Cutter.h"

class CutterSeekable;

class MemoryDockWidget : public CutterDockWidget
{
    Q_OBJECT
public:
    MemoryDockWidget(CutterCore::MemoryWidgetType type, MainWindow *parent, QAction *action = nullptr);
    ~MemoryDockWidget() {}

    CutterSeekable* getSeekable() const;

    bool tryRaiseMemoryWidget();
    void raiseMemoryWidget();
    CutterCore::MemoryWidgetType getType() const
    {
        return mType;
    }
private:

    CutterCore::MemoryWidgetType mType;

public slots:
    void updateWindowTitle();

protected:
    CutterSeekable *seekable = nullptr;

    virtual QString getWindowTitle() const = 0;
};

#endif // MEMORYDOCKWIDGET_H
