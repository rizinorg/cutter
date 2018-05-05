#ifndef SECTIONSWIDGET_H
#define SECTIONSWIDGET_H

#include <memory>

#include <QAbstractListModel>
#include <QSortFilterProxyModel>

#include "Cutter.h"
#include "CutterDockWidget.h"

class QSplitter;
class QTreeView;
class QAbstractItemView;
class QResizeEvent;
class MainWindow;

namespace Ui {
class SectionsWidget;
}

class SectionsModel : public QAbstractListModel
{
    Q_OBJECT

private:
    QList<SectionDescription> *sections;

public:
    enum Column { NameColumn = 0, SizeColumn, AddressColumn, EndAddressColumn, EntropyColumn, ColumnCount };
    enum Role { SectionDescriptionRole = Qt::UserRole };

    SectionsModel(QList<SectionDescription> *sections, QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = QModelIndex()) const;
    int columnCount(const QModelIndex &parent = QModelIndex()) const;

    QVariant data(const QModelIndex &index, int role) const;
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const;

    void beginReloadSections();
    void endReloadSections();
};

class SectionsProxyModel : public QSortFilterProxyModel
{
    Q_OBJECT

public:
    SectionsProxyModel(SectionsModel *sourceModel, QObject *parent = nullptr);

private slots:
    void onSourceModelDataChanged(const QModelIndex& topLeft, const QModelIndex& bottomRight,
                                  const QVector<int>& roles);

protected:
    bool lessThan(const QModelIndex &left, const QModelIndex &right) const override;
};

class SectionsWidget : public CutterDockWidget
{
    Q_OBJECT

public:
    explicit SectionsWidget(MainWindow *main, QAction *action = nullptr);
    ~SectionsWidget();

protected:
    void resizeEvent(QResizeEvent *event) override;

private slots:
    void refreshSections();
    void showSectionsContextMenu(const QPoint &pt);
    void on_actionVertical_triggered();
    void on_actionHorizontal_triggered();
    void onSectionsDoubleClicked(const QModelIndex &index);

private:
    std::unique_ptr<Ui::SectionsWidget> ui;

    QList<SectionDescription> sections;
    SectionsModel *sectionsModel;
    SectionsProxyModel *sectionsProxyModel;
    QSplitter *splitter;
    QTreeView *sectionsTable;
    QAbstractItemView *sectionsPieChart;
    MainWindow *main;

    void setupViews();
};

#endif // SECTIONSWIDGET_H
