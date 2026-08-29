#ifndef GRAPHDIFFWIDGET_H
#define GRAPHDIFFWIDGET_H

#include "CutterDiffWindow.h"
#include "DiffGraphView.h"

#include <QComboBox>
#include <QLabel>
#include <QPushButton>
#include <QWidget>

#include <CutterDiff.h>

enum GraphWidgetMode : ut8 { UnifiedMode, SplitMode, OriginalMode, ModifiedMode };

class GraphDiffWidget : public CutterDiffWidget
{
    Q_OBJECT
public:
    explicit GraphDiffWidget(CutterDiff *cutterDiff, CutterDiffWindow *parent);
    void loadGraph();
    void changeSplitOrientation();
signals:
private:
    CutterDiff *cutterDiff = nullptr;
    DiffGraphView *leftView = nullptr;
    DiffGraphView *rightView = nullptr;
    QComboBox *comboBox = nullptr;
    QLabel *functionLabel = nullptr;
    QPushButton *splitOrientationButton = nullptr;
    bool graphSplitHorizontal = false;
};

#endif // GRAPHDIFFWIDGET_H
