#ifndef SECTIONSWIDGET_H
#define SECTIONSWIDGET_H

#include <QWidget>
#include <QSplitter>
#include <QTreeWidget>

class MainWindow;

QT_BEGIN_NAMESPACE
class QAbstractItemModel;
class QAbstractItemView;
class QItemSelectionModel;
QT_END_NAMESPACE


namespace Ui
{
    class SectionsWidget;
}

class SectionsWidget : public QSplitter
{
    Q_OBJECT

public:
    explicit SectionsWidget(MainWindow *main, QWidget *parent = 0);
    void fillSections(int row, const QString &str, const QString &str2,
                      const QString &str3, const QString &str4);
    void adjustColumns();
    QTreeWidget              *tree;

private:
    //void setupModel();
    void setupViews();

    //QAbstractItemModel     *model;
    QAbstractItemView      *pieChart;
    QItemSelectionModel    *selectionModel;
    MainWindow             *main;
};

#endif // SECTIONSWIDGET_H
