#ifndef GLIBCHEAPWIDGET_H
#define GLIBCHEAPWIDGET_H

#include "CutterDescriptions.h"
#include "CutterDockWidget.h"

#include <QComboBox>
#include <QDockWidget>
#include <QTableView>

#include <AddressableItemContextMenu.h>
#include <memory>

namespace Ui {
class GlibcHeapWidget;
}

/**
 * @brief Source model for @ref GlibcHeapWidget
 */
class GlibcHeapModel : public QAbstractTableModel
{
    Q_OBJECT
public:
    explicit GlibcHeapModel(QObject *parent = nullptr);
    enum Column : ut8 { OffsetColumn = 0, SizeColumn, StatusColumn, ColumnCount };
    void reload();
    int rowCount(const QModelIndex &parent) const override;
    int columnCount(const QModelIndex &parent) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role) const override;
    RVA arenaAddr = 0;

private:
    QVector<Chunk> values;
};

/**
 * @brief Widget listing glibc heap during debugging/emulation
 */
class GlibcHeapWidget : public QWidget
{
    Q_OBJECT

public:
    explicit GlibcHeapWidget(MainWindow *main, QWidget *parent);
    ~GlibcHeapWidget();
private slots:
    void updateContents();
    void onDoubleClicked(const QModelIndex &index);
    void onArenaSelected(int index);
    void customMenuRequested(QPoint pos);
    void onCurrentChanged(const QModelIndex &current, const QModelIndex &previous);
    void viewChunkInfo();
    void viewBinInfo();
    void viewArenaInfo();

private:
    void updateArenas();
    void updateChunks();
    std::unique_ptr<Ui::GlibcHeapWidget> ui;
    QTableView *viewHeap;
    QComboBox *arenaSelectorView;
    GlibcHeapModel *modelHeap = new GlibcHeapModel(this);
    QVector<Arena> arenas;
    QAction *chunkInfoAction;
    QAction *binInfoAction;
    AddressableItemContextMenu addressableItemContextMenu;
    RefreshDeferrer *refreshDeferrer {};
    MainWindow *main;
};

#endif // GLIBCHEAPWIDGET_H
