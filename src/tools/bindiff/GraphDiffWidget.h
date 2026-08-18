#ifndef GRAPHDIFFWIDGET_H
#define GRAPHDIFFWIDGET_H

#include "DiffGraphView.h"

#include <QComboBox>
#include <QLabel>
#include <QPushButton>
#include <QWidget>

#include <CutterDiff.h>

enum GraphWidgetMode : ut8 { UnifiedMode, SplitMode, OriginalMode, ModifiedMode };

class GraphDiffWidget : public QWidget
{
    Q_OBJECT
public:
    explicit GraphDiffWidget(CutterDiff *cutterDiff, QWidget *parent = nullptr);
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
