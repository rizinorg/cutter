#pragma once

#include "CutterDockWidget.h"
#include "Cutter.h"

class MainWindow;

class CutterSeekableWidget : public CutterDockWidget
{

    Q_OBJECT

public:
    explicit CutterSeekableWidget(MainWindow *main, QAction *action = nullptr);
    ~CutterSeekableWidget();
    RVA independentOffset;
    RVA prevIdenpendentOffset;
    bool isInSyncWithCore = true;
    void seek(RVA addr);
    RVA getOffset();

// public slots:
//     void on_seekChanged(RVA)=0;
//     void toggleSync(QString windowTitle, void *callback(RVA));

};