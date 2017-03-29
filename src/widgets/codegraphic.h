#ifndef GRAPHICSBAR_H
#define GRAPHICSBAR_H

#include <QWidget>
#include <QToolBar>
#include <QGraphicsView>

class MainWindow;

class GraphicsBar : public QToolBar
{
    Q_OBJECT

public:
    explicit GraphicsBar(MainWindow *main, QWidget *parent = 0);
    QGraphicsView *codeGraphic;

public slots:
    void paintEvent(QPaintEvent *event);
    void fillData();

private:
    MainWindow      *main;

public slots:

};

#endif // GRAPHICSBAR_H
