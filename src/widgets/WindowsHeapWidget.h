#ifndef WINDOWSHEAPWIDGET_H
#define WINDOWSHEAPWIDGET_H

#include <QWidget>

namespace Ui {
class WindowsHeapWidget;
}

class WindowsHeapWidget : public QWidget
{
    Q_OBJECT

public:
    explicit WindowsHeapWidget(QWidget *parent = nullptr);
    ~WindowsHeapWidget();

private:
    Ui::WindowsHeapWidget *ui;
};

#endif // WINDOWSHEAPWIDGET_H
