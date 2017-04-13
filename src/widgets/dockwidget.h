#ifndef DOCKWIDGET_H
#define DOCKWIDGET_H

#include <QDockWidget>

class DockWidget : public QDockWidget
{
public:
    explicit DockWidget(QWidget *parent = nullptr) :
        QDockWidget(parent) {}
    virtual ~DockWidget() {}

    virtual void setup() = 0;

    virtual void refresh() = 0;
};

#endif // DOCKWIDGET_H
