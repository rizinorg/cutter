#ifndef CUTTERTREEWIDGET_H
#define CUTTERTREEWIDGET_H

#include <QStatusBar>
#include <QVBoxLayout>

class MainWindow;

class CutterTreeWidget : public QObject
{

    Q_OBJECT

public:
    explicit CutterTreeWidget(QObject *parent = nullptr);
    ~CutterTreeWidget();
    void addStatusBar(QVBoxLayout *pos);
    void showItemsNumber(int count);
    void showStatusBar(bool show);

private:
    QStatusBar *bar;

};
#endif // CUTTERTREEWIDGET_H
