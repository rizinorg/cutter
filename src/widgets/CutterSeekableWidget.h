#pragma once

#include "Cutter.h"

class MainWindow;

class CutterSeekableWidget : public QObject
{

    Q_OBJECT

public:
    explicit CutterSeekableWidget(QObject *parent = nullptr);
    ~CutterSeekableWidget();
    void seek(RVA addr);
    void toggleSyncWithCore();
    RVA getOffset();
    RVA getIndependentOffset();
    RVA getPrevIndependentOffset();
    bool getSyncWithCore();
    void setIndependentOffset(RVA addr);
    void onSeekChanged(RVA addr);

private:
    RVA independentOffset = RVA_INVALID;
    RVA prevIdenpendentOffset = RVA_INVALID;
    bool isInSyncWithCore = true;

signals:
    void seekChanged(RVA addr);

};