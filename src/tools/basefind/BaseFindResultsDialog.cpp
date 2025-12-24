#include "BaseFindResultsDialog.h"
#include "ui_BaseFindResultsDialog.h"

#include <QClipboard>
#include <QMessageBox>

#include <core/Cutter.h>
#include <CutterApplication.h>

BaseFindResultsModel::BaseFindResultsModel(QList<BasefindResultDescription> list, QObject *parent)
    : QAbstractListModel(parent), list(list)
{
}

int BaseFindResultsModel::rowCount(const QModelIndex &) const
{
    return list.count();
}

int BaseFindResultsModel::columnCount(const QModelIndex &) const
{
    return BaseFindResultsModel::ColumnCount;
}

QVariant BaseFindResultsModel::data(const QModelIndex &index, int role) const
{
    if (index.row() >= list.count())
        return QVariant();

    const BasefindResultDescription &entry = list.at(index.row());

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
    : QDialog(parent), ui(new Ui::BaseFindResultsDialog)
{
    ui->setupUi(this);
    setWindowFlags(windowFlags() & (~Qt::WindowContextHelpButtonHint));

    model = new BaseFindResultsModel(results, this);
    ui->tableView->setModel(model);
    ui->tableView->sortByColumn(BaseFindResultsModel::ScoreColumn, Qt::AscendingOrder);
    ui->tableView->verticalHeader()->hide();
    ui->tableView->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    ui->tableView->setContextMenuPolicy(Qt::CustomContextMenu);

    blockMenu = new QMenu(this);
    actionCopyCandidate = new QAction(tr("Copy %1"), this);
    actionSetLoadAddr = new QAction(tr("Reopen Cutter with base address as %1"), this);
    actionSetMapAddr = new QAction(tr("Reopen Cutter with map address as %1"), this);

    connect(ui->tableView, &QWidget::customContextMenuRequested, this,
            &BaseFindResultsDialog::showItemContextMenu);
    connect(actionCopyCandidate, &QAction::triggered, this,
            &BaseFindResultsDialog::onActionCopyLine);
    connect(actionSetLoadAddr, &QAction::triggered, this,
            &BaseFindResultsDialog::onActionSetLoadAddr);
    connect(actionSetMapAddr, &QAction::triggered, this,
            &BaseFindResultsDialog::onActionSetMapAddr);

    blockMenu->addAction(actionSetLoadAddr);
    blockMenu->addAction(actionSetMapAddr);
    blockMenu->addAction(actionCopyCandidate);
    addActions(blockMenu->actions());
}

void BaseFindResultsDialog::showItemContextMenu(const QPoint &pt)
{
    auto index = ui->tableView->currentIndex();
    if (index.isValid()) {
        const BasefindResultDescription &entry = model->list.at(index.row());
        candidate = entry.candidate;
        auto addr = QString::asprintf("%#010llx", candidate);
        actionCopyCandidate->setText(tr("Copy %1").arg(addr));
        actionSetLoadAddr->setText(tr("Reopen Cutter with base address as %1").arg(addr));
        actionSetMapAddr->setText(tr("Reopen Cutter with map address as %1").arg(addr));
        blockMenu->exec(this->mapToGlobal(pt));
    }
}

void BaseFindResultsDialog::onActionCopyLine()
{
    auto clipboard = QApplication::clipboard();
    clipboard->setText(QString::asprintf("%#010llx", candidate));
}

void BaseFindResultsDialog::onActionSetLoadAddr()
{
    auto cutter = static_cast<CutterApplication *>(qApp);
    auto options = cutter->getInitialOptions();
    auto oldValue = options.binLoadAddr;

    // override options to generate correct args
    options.binLoadAddr = candidate;
    cutter->setInitialOptions(options);
    auto args = cutter->getArgs();

    // revert back options
    options.binLoadAddr = oldValue;
    cutter->setInitialOptions(options);

    cutter->launchNewInstance(args);
}

void BaseFindResultsDialog::onActionSetMapAddr()
{
    auto cutter = static_cast<CutterApplication *>(qApp);
    auto options = cutter->getInitialOptions();
    auto oldValue = options.mapAddr;

    // override options to generate correct args
    options.mapAddr = candidate;
    cutter->setInitialOptions(options);
    auto args = cutter->getArgs();

    // revert back options
    options.mapAddr = oldValue;
    cutter->setInitialOptions(options);

    cutter->launchNewInstance(args);
}

BaseFindResultsDialog::~BaseFindResultsDialog() {}

void BaseFindResultsDialog::on_buttonBox_rejected() {}
