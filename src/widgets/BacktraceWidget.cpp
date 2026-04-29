#include "BacktraceWidget.h"
#include "ui_BacktraceWidget.h"
#include <QTreeView>

#include "core/MainWindow.h"

BacktraceModel::BacktraceModel(QObject *parent) : QAbstractListModel(parent) {}

int BacktraceModel::rowCount(const QModelIndex & /*parent*/) const
{
    return backtraces.size();
}

int BacktraceModel::columnCount(const QModelIndex & /*parent*/) const
{
    return Column::Count;
}

QVariant BacktraceModel::data(const QModelIndex &index, int role) const
{

    if (!index.isValid() || index.row() >= backtraces.size()) {
        return QVariant();
    }

    if (role == Qt::DisplayRole) {
        switch (index.column()) {
        case Column::Function:
            return backtraces[index.row()].functionName;
        case Column::PC:
            return RzAddressString(backtraces[index.row()].pc);
        case Column::SP:
            return RzAddressString(backtraces[index.row()].sp);
        case Column::FrameSize:
            return backtraces[index.row()].frameSize;
        case Column::Description:
            return backtraces[index.row()].description;
        default:
            return QVariant();
        }
    }

    return QVariant();
}

QVariant BacktraceModel::headerData(int section, Qt::Orientation orientation, int role) const
{

    if (role != Qt::DisplayRole || orientation == Qt::Vertical) {
        return QVariant();
    }

    switch (section) {
    case Column::Function:
        return tr("Function");
    case Column::PC:
        return Core()->getRegisterName("PC");
    case Column::SP:
        return Core()->getRegisterName("SP");
    case Column::FrameSize:
        return tr("Frame Size");
    case Column::Description:
        return tr("Description");
    default:
        return QVariant();
    }
}

void BacktraceModel::setBacktraces(const QList<BacktraceDescription> &backtraces)
{
    beginResetModel();
    this->backtraces = backtraces;
    endResetModel();
}

BacktraceWidget::BacktraceWidget(MainWindow *main)
    : CutterDockWidget(main),
      ui(new Ui::BacktraceWidget),
      backtraceModel(new BacktraceModel(this)),
      backtraceView(new QTreeView(this))
{
    ui->setupUi(this);

    // setup backtrace view
    backtraceView->setFont(Config()->getFont());
    backtraceView->setModel(backtraceModel);
    backtraceView->setHorizontalScrollMode(QAbstractItemView::ScrollPerPixel);
    backtraceView->setIndentation(0);
    ui->verticalLayout->addWidget(backtraceView);

    refreshDeferrer = createRefreshDeferrer([this]() { updateContents(); });

    connect(Core(), &CutterCore::refreshAll, this, &BacktraceWidget::updateContents);
    connect(Core(), &CutterCore::registersChanged, this, &BacktraceWidget::updateContents);
    connect(Config(), &Configuration::fontsUpdated, this, &BacktraceWidget::fontsUpdatedSlot);

    connect(Config(), &Configuration::functionsOptionsChanged, this,
            &BacktraceWidget::adjustFunctionNameCol);
}

BacktraceWidget::~BacktraceWidget() {}

void BacktraceWidget::updateContents()
{
    if (!refreshDeferrer->attemptRefresh(nullptr) || Core()->isDebugTaskInProgress()) {
        return;
    }

    const auto &backtraces = Core()->getAllBacktraces();
    backtraceModel->setBacktraces(backtraces);
    qhelpers::adjustColumns(backtraceView, 1, 3, 0);
    adjustFunctionNameCol();
}

void BacktraceWidget::adjustFunctionNameCol()
{
    qhelpers::adjustColumn(
            backtraceView, BacktraceModel::Function,
            Config()->getTruncateFunctionNameCol() ? Config()->getFunctionNameColWidth() : -1);
}

void BacktraceWidget::fontsUpdatedSlot()
{
    backtraceView->setFont(Config()->getFont());
}
