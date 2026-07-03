#ifndef LINEDIFFWIDGET_H
#define LINEDIFFWIDGET_H

#include <QWidget>
#include <CutterDiff.h>

namespace Ui {
class LineDiffWidget;
}

class LineDiffWidget : public QWidget
{
    Q_OBJECT

public:
    explicit LineDiffWidget(CutterDiff *cutterDiff, QWidget *parent = nullptr);
    ~LineDiffWidget();
    void setData();
private:
    Ui::LineDiffWidget *ui;
    CutterDiff *cutterDiff;
};

#endif // LINEDIFFWIDGET_H
