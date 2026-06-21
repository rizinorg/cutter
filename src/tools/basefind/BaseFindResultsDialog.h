#ifndef BASEFIND_RESULTS_DIALOG_H
#define BASEFIND_RESULTS_DIALOG_H

#include <QAbstractListModel>
#include <QDialog>
#include <QSortFilterProxyModel>

#include <core/Cutter.h>
#include <memory>

class BaseFindResultsDialog;

namespace Ui {
class BaseFindResultsDialog;
}

/**
 * @brief Source model for @ref BaseFindResultsDialog
 */
class BaseFindResultsModel : public QAbstractListModel
{
    Q_OBJECT

    friend BaseFindResultsDialog;

public:
    enum Column : ut8 { ScoreColumn = 0, CandidateColumn, ColumnCount };

    BaseFindResultsModel(QList<BasefindResultDescription> list, QObject *parent = nullptr);

    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const;

    int rowCount(const QModelIndex &parent = QModelIndex()) const;
    int columnCount(const QModelIndex &parent = QModelIndex()) const;

private:
    QList<BasefindResultDescription> list;
};

/**
 * @brief Dialog to display BaseFind results
 *
 * Contains context menu with options to reopen Cutter with base or map address as selected address
 */
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
    void onButtonBoxRejected();

private:
    void onActionCopyLine() const;
    void onActionSetLoadAddr() const;
    void onActionSetMapAddr() const;

    std::unique_ptr<Ui::BaseFindResultsDialog> ui;
    BaseFindResultsModel *model;
    QMenu *blockMenu;
    QAction *actionCopyCandidate;
    QAction *actionSetLoadAddr;
    QAction *actionSetMapAddr;
    RVA candidate;
};

#endif // BASEFIND_RESULTS_DIALOG_H
