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

    bool isSynced() const;

public slots:
    void toggleSync();

private:
    void handleRaiseMemoryWidget(CutterCore::MemoryWidgetType raiseType);

    CutterCore::MemoryWidgetType mType;

protected:
    CutterSeekable *seekable = nullptr;

};

#endif // MEMORYDOCKWIDGET_H
