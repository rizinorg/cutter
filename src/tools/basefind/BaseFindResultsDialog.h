#ifndef BASEFIND_RESULTS_DIALOG_H
#define BASEFIND_RESULTS_DIALOG_H

#include <QDialog>
#include <QAbstractListModel>
#include <QSortFilterProxyModel>
#include <memory>

#include <core/Cutter.h>

class BaseFindResultsDialog;

namespace Ui {
class BaseFindResultsDialog;
}

class BaseFindResultsModel : public QAbstractListModel
{
    Q_OBJECT

    friend BaseFindResultsDialog;

public:
    enum Column { ScoreColumn = 0, CandidateColumn, ColumnCount };

    BaseFindResultsModel(QList<BasefindResultDescription> list, QObject *parent = nullptr);

    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const;

    int rowCount(const QModelIndex &parent = QModelIndex()) const;
    int columnCount(const QModelIndex &parent = QModelIndex()) const;

private:
    QList<BasefindResultDescription> list;
};

class BaseFindResultsDialog : public QDialog
{
    Q_OBJECT

public:
    explicit BaseFindResultsDialog(QList<BasefindResultDescription> results,
                                   QWidget *parent = nullptr);
    ~BaseFindResultsDialog();

public slots:
    void showItemContextMenu(const QPoint &pt);

private slots:
    void on_buttonBox_rejected();

private:
    void onActionCopyLine();
    void onActionSetLoadAddr();
    void onActionSetMapAddr();

    std::unique_ptr<Ui::BaseFindResultsDialog> ui;
    BaseFindResultsModel *model;
    QMenu *blockMenu;
    QAction *actionCopyCandidate;
    QAction *actionSetLoadAddr;
    QAction *actionSetMapAddr;
    RVA candidate;
};

#endif // BASEFIND_RESULTS_DIALOG_H
