#ifndef SECTIONSWIDGET_H
#define SECTIONSWIDGET_H

#include <QSplitter>

class MainWindow;
class QTreeWidget;
class QAbstractItemModel;
class QAbstractItemView;
class QItemSelectionModel;

namespace Ui
{
    class SectionsWidget;
}

class SectionsWidget : public QSplitter
{
    Q_OBJECT

public:
    explicit SectionsWidget(MainWindow *main, QWidget *parent = 0);

    void setup();

private:
    QAbstractItemView       *pieChart;
    QItemSelectionModel     *selectionModel;
    MainWindow              *main;
    QTreeWidget             *tree;

    void setupViews();

    void fillSections(int row, const QString &str, const QString &str2 = QString(),
                      const QString &str3 = QString(), const QString &str4 = QString());
    void adjustColumns();
};

#endif // SECTIONSWIDGET_H
