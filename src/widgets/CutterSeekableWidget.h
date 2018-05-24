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
    RVA independentOffset = RVA_INVALID;
    RVA prevIdenpendentOffset = RVA_INVALID;
    bool isInSyncWithCore = true;
    void seek(RVA addr);
    RVA getOffset();

signals:
    void seekChanged(RVA addr);

};