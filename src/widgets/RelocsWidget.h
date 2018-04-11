#ifndef RELOCSWIDGET_H
#define RELOCSWIDGET_H

#include <memory>
#include <QAbstractTableModel>

#include "CutterDockWidget.h"
#include "Cutter.h"

class MainWindow;

namespace Ui {
class RelocsWidget;
}

class RelocsModel : public QAbstractTableModel
{
    Q_OBJECT

private:
    QList<RelocDescription> *relocs;

public:
    enum COLUMNS {VADDR = 0, TYPE, NAME, COUNT};
    static const int AddressRole = Qt::UserRole;

    RelocsModel(QList<RelocDescription> *relocs, QObject* parent = nullptr);

    int rowCount(const QModelIndex &parent) const;
    int columnCount(const QModelIndex &parent) const;

    QVariant data(const QModelIndex &index, int role) const;
    QVariant headerData(int section, Qt::Orientation orientation, int role) const;

    void beginReload();
    void endReload();
};

class RelocsWidget : public CutterDockWidget
{
    Q_OBJECT

public:
    explicit RelocsWidget(MainWindow *main, QAction *action = nullptr);
    ~RelocsWidget();

private slots:
    void on_relocsTableView_doubleClicked(const QModelIndex &index);
    void refreshRelocs();

private:
    std::unique_ptr<Ui::RelocsWidget> ui;
    RelocsModel *model;
    QList<RelocDescription> relocs;

    void setScrollMode();
};

#endif // RELOCSWIDGET_H
