#ifndef STRINGSWIDGET_H
#define STRINGSWIDGET_H

#include <QDockWidget>
#include <QTreeWidget>

class MainWindow;

namespace Ui {
class StringsWidget;
}

class StringsWidget : public QDockWidget
{
    Q_OBJECT

public:
    explicit StringsWidget(MainWindow *main, QWidget *parent = 0);
    ~StringsWidget();

    QTreeWidget    *stringsTreeWidget;

private slots:
    void on_stringsTreeWidget_itemDoubleClicked(QTreeWidgetItem *item, int column);

private:
    Ui::StringsWidget *ui;

    MainWindow      *main;
};

#endif // STRINGSWIDGET_H
