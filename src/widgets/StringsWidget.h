#ifndef STRINGSWIDGET_H
#define STRINGSWIDGET_H

#include <memory>

#include <QDockWidget>

class MainWindow;
class QTreeWidgetItem;

namespace Ui
{
    class StringsWidget;
}

class StringsWidget : public QDockWidget
{
    Q_OBJECT

public:
    explicit StringsWidget(MainWindow *main, QWidget *parent = 0);
    ~StringsWidget();

private slots:
    void on_stringsTreeWidget_itemDoubleClicked(QTreeWidgetItem *item, int column);

    void fillTreeWidget();

private:
    std::unique_ptr<Ui::StringsWidget> ui;
    MainWindow      *main;

    void setScrollMode();
};

#endif // STRINGSWIDGET_H
