#ifndef SECTIONSWIDGET_H
#define SECTIONSWIDGET_H

#include <QSplitter>

class MainWindow;
class QTreeWidget;
class QAbstractItemModel;
class QAbstractItemView;
class QItemSelectionModel;

struct SectionDescription;

namespace Ui {
class SectionsWidget;
}

class SectionsWidget : public QSplitter
{
    Q_OBJECT

public:
    explicit SectionsWidget(MainWindow *main, QWidget *parent = 0);

private slots:
    void refreshSections();

    void onSectionsDoubleClicked(const QModelIndex &index);

private:
    QAbstractItemView       *pieChart;
    QItemSelectionModel     *selectionModel;
    MainWindow              *main;
    QTreeWidget             *tree;

    void setupViews();

    void fillSections(int row, const SectionDescription &section);
};

#endif // SECTIONSWIDGET_H
