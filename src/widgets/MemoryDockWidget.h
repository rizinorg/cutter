#ifndef MEMORYDOCKWIDGET_H
#define MEMORYDOCKWIDGET_H

#include "CutterDockWidget.h"
#include "core/Cutter.h"

class MemoryDockWidget : public CutterDockWidget
{
    Q_OBJECT
public:
    MemoryDockWidget(CutterCore::MemoryWidgetType type, MainWindow *parent, QAction *action = nullptr);
    ~MemoryDockWidget() {}

private:
    void handleRaiseMemoryWidget(CutterCore::MemoryWidgetType raiseType);

    CutterCore::MemoryWidgetType mType;
};

#endif // MEMORYDOCKWIDGET_H
