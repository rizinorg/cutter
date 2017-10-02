#ifndef GRAPHICSBAR_H
#define GRAPHICSBAR_H

#include <QToolBar>

class MainWindow;
class QGraphicsView;

class GraphicsBar : public QToolBar
{
    Q_OBJECT

public:
    explicit GraphicsBar(MainWindow *main, QWidget *parent = 0);

public slots:
    void paintEvent(QPaintEvent *event);
    void fillData();

private:
    QGraphicsView   *codeGraphic;
    MainWindow      *main;
};

#endif // GRAPHICSBAR_H
