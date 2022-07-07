#include "BaseFindResultsDialog.h"
#include "ui_BaseFindResultsDialog.h"

#include <QClipboard>
#include <QMessageBox>

#include <core/Cutter.h>

BaseFindResultsModel::BaseFindResultsModel(QList<BasefindResultDescription> *list, QObject *parent)
    : QAbstractListModel(parent), list(list)
{
}

int BaseFindResultsModel::rowCount(const QModelIndex &) const
{
    return list->count();
}

int BaseFindResultsModel::columnCount(const QModelIndex &) const
{
    return BaseFindResultsModel::ColumnCount;
}

QVariant BaseFindResultsModel::data(const QModelIndex &index, int role) const
{
    if (index.row() >= list->count())
        return QVariant();

    const BasefindResultDescription &entry = list->at(index.row());

    switch (role) {
    case Qt::DisplayRole:
        switch (index.column()) {
        case ScoreColumn:
            return QString::asprintf("%u", entry.score);
        case CandidateColumn:
            return QString::asprintf("%#010llx", entry.candidate);
        default:
            return QVariant();
        }

    case Qt::ToolTipRole: {
        return QString::asprintf("%#010llx", entry.candidate);
    }

    default:
        return QVariant();
    }
}

QVariant BaseFindResultsModel::headerData(int section, Qt::Orientation, int role) const
{
    switch (role) {
    case Qt::DisplayRole:
        switch (section) {
        case ScoreColumn:
            return tr("Score");
        case CandidateColumn:
            return tr("Address");
        default:
            return QVariant();
        }
    default:
        return QVariant();
    }
}

BaseFindResultsDialog::BaseFindResultsDialog(QList<BasefindResultDescription> results,
                                             QWidget *parent)
    : QDialog(parent), list(results), ui(new Ui::BaseFindResultsDialog)
{
    ui->setupUi(this);
    setWindowFlags(windowFlags() & (~Qt::WindowContextHelpButtonHint));

    model = new BaseFindResultsModel(&list, this);
    ui->tableView->setModel(model);
    ui->tableView->sortByColumn(BaseFindResultsModel::ScoreColumn, Qt::AscendingOrder);
    ui->tableView->verticalHeader()->hide();
    ui->tableView->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    ui->tableView->setContextMenuPolicy(Qt::CustomContextMenu);

    blockMenu = new QMenu(this);
    actionCopyCandidate = new QAction(tr("Copy %1"), this);
    actionSetBaseAddr = new QAction(tr("Set base address as %1"), this);

    connect(ui->tableView, &QWidget::customContextMenuRequested, this,
            &BaseFindResultsDialog::showItemContextMenu);
    connect(actionCopyCandidate, &QAction::triggered, this,
            &BaseFindResultsDialog::onActionCopyLine);
    connect(actionSetBaseAddr, &QAction::triggered, this,
            &BaseFindResultsDialog::onActionSetRebaseAddr);

    blockMenu->addAction(actionSetBaseAddr);
    blockMenu->addAction(actionCopyCandidate);
    addActions(blockMenu->actions());
}

void BaseFindResultsDialog::showItemContextMenu(const QPoint &pt)
{
    auto index = ui->tableView->currentIndex();
    if (index.isValid()) {
        const BasefindResultDescription &entry = list.at(index.row());
        candidate = entry.candidate;
        auto addr = QString::asprintf("%#010llx", candidate);
        actionCopyCandidate->setText(tr("Copy %1").arg(addr));
        actionSetBaseAddr->setText(tr("Set base address as %1").arg(addr));
        blockMenu->exec(this->mapToGlobal(pt));
    }
}

void BaseFindResultsDialog::onActionCopyLine()
{
    auto clipboard = QApplication::clipboard();
    clipboard->setText(QString::asprintf("%#010llx", candidate));
}

void BaseFindResultsDialog::onActionSetRebaseAddr()
{
    QString message;
    QMessageBox mbox;
    mbox.setWindowTitle("BaseFind");
    if (Core()->rebaseBin(candidate)) {
        message = tr("Binary successfully rebased at %1.")
                          .arg(QString::asprintf("%#010llx", candidate));
        mbox.setIcon(QMessageBox::Information);
    } else {
        auto message =
                tr("Failed to rebase binary at %1.").arg(QString::asprintf("%#010llx", candidate));
        mbox.setIcon(QMessageBox::Warning);
    }
    mbox.setText(message);
    mbox.exec();
}

BaseFindResultsDialog::~BaseFindResultsDialog() {}

void BaseFindResultsDialog::on_buttonBox_rejected() {}
