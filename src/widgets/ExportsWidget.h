#ifndef EXPORTSWIDGET_H
#define EXPORTSWIDGET_H

#include <memory>

#include "core/Cutter.h"
#include "CutterDockWidget.h"
#include "CutterTreeWidget.h"

#include <QAbstractListModel>
#include <QSortFilterProxyModel>

class MainWindow;
class QTreeWidget;
class ExportsWidget;

namespace Ui {
class ExportsWidget;
}

class ExportsModel : public QAbstractListModel
{
    Q_OBJECT

    friend ExportsWidget;

private:
    QList<ExportDescription> *exports;

public:
    enum Column { OffsetColumn = 0, SizeColumn, TypeColumn, NameColumn, ColumnCount };
    enum Role { ExportDescriptionRole = Qt::UserRole };

    ExportsModel(QList<ExportDescription> *exports, QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = QModelIndex()) const;
    int columnCount(const QModelIndex &parent = QModelIndex()) const;

    QVariant data(const QModelIndex &index, int role) const;
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const;
};

class ExportsProxyModel : public QSortFilterProxyModel
{
    Q_OBJECT

public:
    ExportsProxyModel(ExportsModel *source_model, QObject *parent = nullptr);

protected:
    bool filterAcceptsRow(int row, const QModelIndex &parent) const override;
    bool lessThan(const QModelIndex &left, const QModelIndex &right) const override;
};

class ExportsWidget : public CutterDockWidget
{
    Q_OBJECT

public:
    explicit ExportsWidget(MainWindow *main, QAction *action = nullptr);
    ~ExportsWidget();

private slots:
    void on_exportsTreeView_doubleClicked(const QModelIndex &index);

    void refreshExports();

private:
    std::unique_ptr<Ui::ExportsWidget> ui;

    ExportsModel *exportsModel;
    ExportsProxyModel *exportsProxyModel;
    QList<ExportDescription> exports;
    CutterTreeWidget *tree;

    void setScrollMode();
};

#endif // EXPORTSWIDGET_H
