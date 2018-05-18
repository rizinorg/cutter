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
    bool isInSyncWithCore = true;
    void seek(RVA addr);

// public slots:
//     void on_seekChanged(RVA)=0;

// public slots:
//     void toggleSync(QString windowTitle, void *callback(RVA));

};