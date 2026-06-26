#include "RizinConfigOptionsWidget.h"

#include "Cutter.h"
#include "dialogs/IntervalDialog.h"
#include "dialogs/StringListDialog.h"
#include "ui_RizinConfigOptionsWidget.h"

#include <QComboBox>
#include <QFormLayout>
#include <QTimer>
#include <QVBoxLayout>

namespace {

QModelIndex siblingAtColumn(const QModelIndex &idx, int column)
{
#if QT_VERSION >= QT_VERSION_CHECK(5, 11, 0)
    return idx.siblingAtColumn(column);
#else
    return idx.sibling(idx.row(), column);
#endif
}

}

RizinConfigOptionsModel::RizinConfigOptionsModel(QObject *parent) : QAbstractItemModel(parent)
{
    refresh();
}

int RizinConfigOptionsModel::rowCount(const QModelIndex &parent) const
{
    if (parent.column() > NameColumn) {
        return 0;
    }

    if (!parent.isValid()) {
        return categories.count();
    }

    if (parent.internalId() == internalCategoryMarker) {
        return evalVarsMap.value(categories.at(parent.row())).count();
    }

    return 0;
}

int RizinConfigOptionsModel::columnCount(const QModelIndex & /*parent*/) const
{
    return ColumnCount;
}

QModelIndex RizinConfigOptionsModel::index(int row, int column, const QModelIndex &parent) const
{
    if (!hasIndex(row, column, parent)) {
        return QModelIndex();
    }

    if (!parent.isValid()) {
        return createIndex(row, column, internalCategoryMarker);
    }

    return createIndex(row, column, parent.row());
}

QModelIndex RizinConfigOptionsModel::parent(const QModelIndex &index) const
{
    if (!index.isValid() || index.internalId() == internalCategoryMarker) {
        return QModelIndex();
    }

    return createIndex(index.internalId(), 0, internalCategoryMarker);
}

QVariant RizinConfigOptionsModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid()) {
        return QVariant();
    }

    const bool isCategory = (index.internalId() == internalCategoryMarker);

    if (isCategory) {
        switch (role) {
        case Qt::DisplayRole: {
            if (index.column() == NameColumn) {
                return categories.at(index.row());
            }
            break;
        }
        case Qt::FontRole: {
            QFont font;
            font.setBold(true);
            return font;
        }
        case Qt::ForegroundRole: {
            return qApp->palette().color(QPalette::Active, QPalette::Text);
        }
        }
        return QVariant();
    }

    const auto &items = evalVarsMap.value(categories.at(index.internalId()));
    if (index.row() >= items.count()) {
        return QVariant();
    }
    const auto &evalVar = items.at(index.row());

    if (evalVar.type == EvaluableVarDescription::Bool && index.column() == ValueColumn) {
        if (role == Qt::CheckStateRole) {
            return (evalVar.value == "true" || evalVar.value == "1") ? Qt::Checked : Qt::Unchecked;
        } else if (role == Qt::DisplayRole || role == Qt::EditRole) {
            return QVariant();
        }
    }

    switch (role) {
    case Qt::DisplayRole:
    case Qt::EditRole: {
        switch (index.column()) {
        case NameColumn:
            return evalVar.name;
        case ValueColumn:
            if (evalVar.type == EvaluableVarDescription::Interval) {
                const auto itv = evalVar.value.value<RzInterval>();
                return QString("%1+%2").arg(rzAddressString(itv.addr), rzSizeString(itv.size));
            }
            if (evalVar.type == EvaluableVarDescription::List) {
                const QStringList list = evalVar.value.toStringList();
                if (list.isEmpty()) {
                    return QString();
                }
                return list.count() > 1 ? list.first() + "..." : list.first();
            }
            return evalVar.value;
        case DescColumn:
            return evalVar.description;
        }
        return QVariant();
    }
    case EvaluableVarRole: {
        return QVariant::fromValue<EvaluableVarDescription>(evalVar);
    }
    default:
        return QVariant();
    }
}

QVariant RizinConfigOptionsModel::headerData(int section, Qt::Orientation /*orientation*/,
                                             int role) const
{
    if (role == Qt::DisplayRole) {
        switch (section) {
        case NameColumn:
            return tr("Name");
        case ValueColumn:
            return tr("Value");
        case DescColumn:
            return tr("Description");
        }
    }

    return QVariant();
}

Qt::ItemFlags RizinConfigOptionsModel::flags(const QModelIndex &index) const
{
    if (!index.isValid() || index.internalId() >= static_cast<qulonglong>(categories.count())) {
        return Qt::NoItemFlags;
    }

    if (index.internalId() == internalCategoryMarker) {
        return Qt::ItemIsSelectable | Qt::ItemIsEnabled;
    }

    Qt::ItemFlags defaultFlags = Qt::ItemIsSelectable | Qt::ItemIsEnabled;

    const auto &items = evalVarsMap.value(categories.at(index.internalId()));
    const auto evalVar = items.at(index.row());
    if (index.column() == ValueColumn && !evalVar.readOnly) {
        if (evalVar.type == EvaluableVarDescription::Bool) {
            defaultFlags |= Qt::ItemIsUserCheckable;
        } else {
            defaultFlags |= Qt::ItemIsEditable;
        }
    }

    return defaultFlags;
}

bool RizinConfigOptionsModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    if (!index.isValid() || index.column() != ValueColumn) {
        return false;
    }

    const QString categoryKey = categories.at(index.internalId());
    if (!evalVarsMap.contains(categoryKey)) {
        return false;
    }

    auto &items = evalVarsMap[categoryKey];
    if (index.row() >= items.count()) {
        return false;
    }

    auto &evalVar = items[index.row()];

    if (evalVar.type == EvaluableVarDescription::Bool && role == Qt::CheckStateRole) {
        evalVar.value = (value.toInt() == Qt::Checked) ? "true" : "false";
        emit dataChanged(index, index, { Qt::DisplayRole, Qt::EditRole, Qt::CheckStateRole });
        return true;
    } else if (evalVar.type == EvaluableVarDescription::Int && role == Qt::EditRole) {
        const QString res = value.toString();
        bool ok = false;
        res.toULongLong(&ok, res.startsWith("0x", Qt::CaseInsensitive) ? 16 : 10);
        if (!ok) {
            return false;
        }
    }

    if (role != Qt::EditRole) {
        return false;
    }

    evalVar.value = value;

    emit dataChanged(index, index, { Qt::DisplayRole, Qt::EditRole });
    return true;
}

void RizinConfigOptionsModel::refresh()
{
    beginResetModel();

    categories.clear();
    evalVarsMap.clear();

    const auto evalVars = Core()->getAllEvaluableVars();
    categories = Core()->getAllEvaluableVarSpaces();

    for (const auto &var : evalVars) {
        const QString prefix = var.name.section('.', 0, 0);
        evalVarsMap[prefix].append(var);
    }

    endResetModel();
}

QList<QString> RizinConfigOptionsModel::getCategories() const
{
    return categories;
}

RizinConfigOptionsProxyModel::RizinConfigOptionsProxyModel(RizinConfigOptionsModel *sourceModel,
                                                           QObject *parent)
    : QSortFilterProxyModel(parent)
{
    setSourceModel(sourceModel);
}

void RizinConfigOptionsProxyModel::setSelectedSpace(const QString &space)
{
    if (this->space != space) {
#if QT_VERSION >= QT_VERSION_CHECK(6, 10, 0)
        beginFilterChange();
        this->space = space;
        endFilterChange();
#else
        this->space = space;
        invalidateFilter();
#endif
    }
}

bool RizinConfigOptionsProxyModel::filterAcceptsRow(int row, const QModelIndex &parent) const
{
    const QModelIndex index = sourceModel()->index(row, 0, parent);
    if (!index.isValid()) {
        return false;
    }

    if (!space.isEmpty() && space != "All") {
        QString categoryName;

        if (!parent.isValid()) {
            categoryName = index.data(Qt::DisplayRole).toString();
        } else {
            categoryName = parent.data(Qt::DisplayRole).toString();
        }

        if (categoryName != space) {
            return false;
        }
    }

    auto matches = [this](const QModelIndex &idx) {
        const QString name = idx.data(Qt::DisplayRole).toString();
        const QString desc = siblingAtColumn(idx, RizinConfigOptionsModel::DescColumn)
                                     .data(Qt::DisplayRole)
                                     .toString();

        return qhelpers::filterStringContains(name, this)
                || qhelpers::filterStringContains(desc, this);
    };

    if (sourceModel()->rowCount(index) > 0) {
        const int childCount = sourceModel()->rowCount(index);
        for (int i = 0; i < childCount; ++i) {
            const QModelIndex childIdx = sourceModel()->index(i, 0, index);
            if (matches(childIdx)) {
                return true;
            }
        }
    }

    return matches(index);
}

bool RizinConfigOptionsProxyModel::lessThan(const QModelIndex &left, const QModelIndex &right) const
{
    const QVariant leftData = sourceModel()->data(left, Qt::DisplayRole);
    const QVariant rightData = sourceModel()->data(right, Qt::DisplayRole);

#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    if (leftData.typeId() == QMetaType::Int) {
        return leftData.toInt() < rightData.toInt();
    }
#else
    if (leftData.type() == QVariant::Int) {
        return leftData.toInt() < rightData.toInt();
    }
#endif

    return leftData.toString().localeAwareCompare(rightData.toString()) < 0;
}

QWidget *RizinConfigOptionsDelegate::createEditor(QWidget *parent,
                                                  const QStyleOptionViewItem &option,
                                                  const QModelIndex &index) const
{
    const auto evalVar =
            index.data(RizinConfigOptionsModel::EvaluableVarRole).value<EvaluableVarDescription>();

    if (evalVar.type == EvaluableVarDescription::Bool
        || evalVar.type == EvaluableVarDescription::Interval
        || evalVar.type == EvaluableVarDescription::List) {
        // handled in lambda  connected to ui->treeView::doubleClicked
        return nullptr;
    }

    if (!evalVar.options.isEmpty()) {
        auto *comboBox = new QComboBox(parent);
        for (const auto &opt : evalVar.options) {
            comboBox->addItem(opt);
        }
        return comboBox;
    }

    return QStyledItemDelegate::createEditor(parent, option, index);
}

void RizinConfigOptionsDelegate::setEditorData(QWidget *editor, const QModelIndex &index) const
{
    const auto evalVar =
            index.data(RizinConfigOptionsModel::EvaluableVarRole).value<EvaluableVarDescription>();

    if (auto *comboBox = qobject_cast<QComboBox *>(editor)) {
        const QString currentText = index.data(Qt::EditRole).toString();

        const int comboIndex = comboBox->findText(currentText);

        if (comboIndex >= 0) {
            comboBox->setCurrentIndex(comboIndex);
        } else {
            comboBox->addItem(currentText);
            comboBox->setCurrentIndex(comboBox->count() - 1);
        }
    } else if (auto *intervalDialog = qobject_cast<IntervalDialog *>(editor)) {
        const auto itv = evalVar.value.value<RzInterval>();
        intervalDialog->setAddress(itv.addr);
        intervalDialog->setSize(itv.size);
    } else {
        QStyledItemDelegate::setEditorData(editor, index);
    }
}

void RizinConfigOptionsDelegate::setModelData(QWidget *editor, QAbstractItemModel *model,
                                              const QModelIndex &index) const
{
    if (auto *comboBox = qobject_cast<QComboBox *>(editor)) {
        model->setData(index, comboBox->currentText(), Qt::EditRole);
    } else if (auto *intervalDialog = qobject_cast<IntervalDialog *>(editor)) {
        if (intervalDialog->result() == QDialog::Accepted) {
            bool okAddr = false;
            bool okSize = false;

            const QString addrStr = intervalDialog->getAddress();
            const QString sizeStr = intervalDialog->getSize();

            const ut64 addr = addrStr.toULongLong(&okAddr, addrStr.startsWith("0x") ? 16 : 10);
            const ut64 size = sizeStr.toULongLong(&okSize, sizeStr.startsWith("0x") ? 16 : 10);

            if (okAddr && okSize) {
                const RzInterval itv { addr, size };
                model->setData(index, QVariant::fromValue(itv), Qt::EditRole);
            }
        }
    } else {
        QStyledItemDelegate::setModelData(editor, model, index);
    }
}

RizinConfigOptionsWidget::RizinConfigOptionsWidget(PreferencesDialog *parent)
    : QDialog(parent),
      ui(new Ui::RizinConfigOptionsWidget),
      sourceModel(new RizinConfigOptionsModel(this)),
      proxyModel(new RizinConfigOptionsProxyModel(sourceModel, this)),
      delegate(new RizinConfigOptionsDelegate(this))
{
    ui->setupUi(this);
    ui->treeView->setModel(proxyModel);
    ui->treeView->setItemDelegate(delegate);

    ui->treeView->setSortingEnabled(true);
    ui->treeView->header()->setSectionsClickable(true);
    ui->treeView->sortByColumn(RizinConfigOptionsModel::NameColumn, Qt::AscendingOrder);
    ui->treeView->expandAll();

    qhelpers::adjustColumns(ui->treeView, RizinConfigOptionsModel::ColumnCount, 0);
    qhelpers::adjustColumn(ui->treeView, RizinConfigOptionsModel::ValueColumn, 120);

    ui->comboQuickFilter->setLabelText("Space");

    connect(ui->treeView, &QTreeView::doubleClicked, this, [this](const QModelIndex &index) {
        if (!index.isValid()) {
            return;
        }

        const QModelIndex srcIndex = proxyModel->mapToSource(index);
        if (srcIndex.internalId() == RizinConfigOptionsModel::internalCategoryMarker) {
            return;
        }

        auto evalVar = index.data(RizinConfigOptionsModel::EvaluableVarRole)
                               .value<EvaluableVarDescription>();
        if (evalVar.type == EvaluableVarDescription::Bool) {
            const QModelIndex valueColumnIndex =
                    siblingAtColumn(index, RizinConfigOptionsModel::ValueColumn);
            const Qt::CheckState currentState =
                    static_cast<Qt::CheckState>(valueColumnIndex.data(Qt::CheckStateRole).toInt());
            const Qt::CheckState newState =
                    (currentState == Qt::Checked) ? Qt::Unchecked : Qt::Checked;
            proxyModel->setData(valueColumnIndex, newState, Qt::CheckStateRole);
            return;
        }

        if (evalVar.type == EvaluableVarDescription::Interval) {
            IntervalDialog dialog(evalVar.name, this);
            const auto itv = evalVar.value.value<RzInterval>();
            dialog.setAddress(itv.addr);
            dialog.setSize(itv.size);

            auto pos = QCursor::pos();
            pos.setX(pos.x() - dialog.width());
            pos.setY(pos.y() - dialog.height() / 2);
            dialog.move(pos);

            if (dialog.exec() == QDialog::Accepted) {
                bool okAddr = false;
                bool okSize = false;
                const QString addrStr = dialog.getAddress();
                const QString sizeStr = dialog.getSize();

                const ut64 addr = addrStr.toULongLong(&okAddr, addrStr.startsWith("0x") ? 16 : 10);
                const ut64 size = sizeStr.toULongLong(&okSize, sizeStr.startsWith("0x") ? 16 : 10);

                if (okAddr && okSize) {
                    const RzInterval newItv { addr, size };
                    const QModelIndex valueColumnIndex =
                            siblingAtColumn(index, RizinConfigOptionsModel::ValueColumn);
                    proxyModel->setData(valueColumnIndex, QVariant::fromValue(newItv),
                                        Qt::EditRole);
                }
            }
            return;
        }

        if (evalVar.type == EvaluableVarDescription::List) {
            StringListDialog dialog(evalVar.value.toStringList(), this);
            dialog.setWindowTitle(tr("Edit List for %1").arg(evalVar.name));

            auto pos = QCursor::pos();
            pos.setX(pos.x() - dialog.width());
            pos.setY(pos.y() - dialog.height() / 2);
            dialog.move(pos);

            if (dialog.exec() == QDialog::Accepted) {
                const QModelIndex valueColumnIndex =
                        siblingAtColumn(index, RizinConfigOptionsModel::ValueColumn);
                proxyModel->setData(valueColumnIndex, dialog.getStringList(), Qt::EditRole);
            }
            return;
        }

        if (index.column() != RizinConfigOptionsModel::ValueColumn) {
            const QModelIndex valueColumnIndex =
                    siblingAtColumn(index, RizinConfigOptionsModel::ValueColumn);
            ui->treeView->edit(valueColumnIndex);
        } else {
            ui->treeView->edit(index);
        }
    });

    connect(ui->comboQuickFilter, &ComboQuickFilterView::filterTextChanged, this,
            [this](const QString &text) {
                proxyModel->setFilterWildcard(text);
                updateItemCount();
                ui->treeView->expandAll();
            });

    connect(ui->comboQuickFilter->comboBox(), &QComboBox::currentTextChanged, this, [this]() {
        proxyModel->setSelectedSpace(ui->comboQuickFilter->comboBox()->currentData().toString());
        updateItemCount();
        ui->treeView->expandAll();
    });

    connect(proxyModel, &QSortFilterProxyModel::dataChanged, this,
            &RizinConfigOptionsWidget::handleConfigOptionChanged);

    connect(ui->analyzeBtn, &QPushButton::clicked, parent->getMainWindow(),
            &MainWindow::onActionAnalyzeTriggered);

    connect(ui->defaultBtn, &QPushButton::clicked, this,
            &RizinConfigOptionsWidget::resetToDefaultBtnPressed);
    connect(ui->restoreBtn, &QPushButton::clicked, this,
            &RizinConfigOptionsWidget::restoreBtnPressed);

    // refresh
    connect(Core(), &CutterCore::refreshAll, this, &RizinConfigOptionsWidget::refresh);
    connect(Core(), &CutterCore::asmOptionsChanged, this, &RizinConfigOptionsWidget::refresh);
    connect(Core(), &CutterCore::debugOptionsChanged, this, &RizinConfigOptionsWidget::refresh);
    connect(Core(), &CutterCore::analysisOptionsChanged, this, &RizinConfigOptionsWidget::refresh);
    connect(Core(), &CutterCore::symbolsOptionsChanged, this, &RizinConfigOptionsWidget::refresh);

    refreshComboQuickFilter();
    updateItemCount();
}

RizinConfigOptionsWidget::~RizinConfigOptionsWidget() {}

void RizinConfigOptionsWidget::refresh()
{
    sourceModel->refresh();
    refreshComboQuickFilter();
    updateItemCount();
    ui->treeView->expandAll();
}

void RizinConfigOptionsWidget::refreshComboQuickFilter()
{
    auto comboBox = ui->comboQuickFilter->comboBox();
    comboBox->clear();
    comboBox->addItem(tr("All"), "All");
    for (const auto &category : sourceModel->getCategories()) {
        comboBox->addItem(category, category);
    }
    ui->comboQuickFilter->setItemCount(proxyModel->rowCount());
}

void RizinConfigOptionsWidget::updateItemCount()
{
    int totalVisibleItems = 0;
    const int visibleCategories = proxyModel->rowCount();

    for (int i = 0; i < visibleCategories; ++i) {
        const QModelIndex categoryIndex = proxyModel->index(i, 0);
        totalVisibleItems += proxyModel->rowCount(categoryIndex);
    }
    ui->comboQuickFilter->setItemCount(totalVisibleItems);
}

void RizinConfigOptionsWidget::handleConfigOptionChanged(const QModelIndex &topLeft,
                                                         const QModelIndex & /*bottomRight*/,
                                                         const QVector<int> &roles)
{
    if (!roles.contains(Qt::EditRole) && !roles.contains(Qt::CheckStateRole)) {
        return;
    }

    const QModelIndex proxyIndex = topLeft;

    auto evalVar = proxyIndex.data(RizinConfigOptionsModel::EvaluableVarRole)
                           .value<EvaluableVarDescription>();

    if (!originalValues.contains(evalVar.name)) {
        if (evalVar.type == EvaluableVarDescription::Interval) {
            originalValues[evalVar.name] = QVariant::fromValue(Core()->getConfigItv(evalVar.name));
        } else if (evalVar.type == EvaluableVarDescription::List) {
            originalValues[evalVar.name] = QVariant::fromValue(Core()->getConfigList(evalVar.name));
        } else {
            originalValues[evalVar.name] = Core()->getConfig(evalVar.name);
        }
    }

    Core()->setConfig(evalVar.name, evalVar.value);
    triggerOptionsChanged(getOptionChangedFromEvalVar(evalVar.name));

    ui->treeView->expandAll();
}

void RizinConfigOptionsWidget::triggerOptionsChanged(int options) const
{
    if (options & OptionChanged::Asm) {
        disconnect(Core(), &CutterCore::asmOptionsChanged, this,
                   &RizinConfigOptionsWidget::refresh);
        Core()->triggerAsmOptionsChanged();
        connect(Core(), &CutterCore::asmOptionsChanged, this, &RizinConfigOptionsWidget::refresh);
    }
    if (options & OptionChanged::Debug) {
        disconnect(Core(), &CutterCore::debugOptionsChanged, this,
                   &RizinConfigOptionsWidget::refresh);
        Core()->triggerDebugOptionsChanged();
        connect(Core(), &CutterCore::debugOptionsChanged, this, &RizinConfigOptionsWidget::refresh);
    }
    if (options & OptionChanged::Analysis) {
        disconnect(Core(), &CutterCore::analysisOptionsChanged, this,
                   &RizinConfigOptionsWidget::refresh);
        Core()->triggerAnalysisOptionsChanged();
        connect(Core(), &CutterCore::analysisOptionsChanged, this,
                &RizinConfigOptionsWidget::refresh);
    }
    if (options & OptionChanged::Symbols) {
        disconnect(Core(), &CutterCore::symbolsOptionsChanged, this,
                   &RizinConfigOptionsWidget::refresh);
        Core()->triggerSymbolsOptionsChanged();
        connect(Core(), &CutterCore::symbolsOptionsChanged, this,
                &RizinConfigOptionsWidget::refresh);
    }
}

RizinConfigOptionsWidget::OptionChanged
RizinConfigOptionsWidget::getOptionChangedFromEvalVar(const QString &name)
{
    if (name.startsWith("asm")) {
        return OptionChanged::Asm;
    } else if (name.startsWith("dbg") || name.startsWith("esil")) {
        return OptionChanged::Debug;
    } else if (name.startsWith("analysis")) {
        return OptionChanged::Analysis;
    } else if (name.startsWith("pdb") || name.startsWith("bin.dbginfo")) {
        return OptionChanged::Symbols;
    }
    return OptionChanged::Invalid;
}

void RizinConfigOptionsWidget::resetToDefaultBtnPressed()
{
    Core()->resetConfig();
    triggerOptionsChanged(OptionChanged::All);
    Core()->triggerRefreshAll();
    updateItemCount();
    ui->treeView->expandAll();
}

void RizinConfigOptionsWidget::restoreBtnPressed()
{
    const QModelIndexList selectedRows = ui->treeView->selectionModel()->selectedRows();

    int options = 0;

    for (const QModelIndex &index : selectedRows) {
        const QModelIndex srcIndex = proxyModel->mapToSource(index);
        if (srcIndex.internalId() == RizinConfigOptionsModel::internalCategoryMarker) {
            continue;
        }

        auto evalVar = index.data(RizinConfigOptionsModel::EvaluableVarRole)
                               .value<EvaluableVarDescription>();
        if (!evalVar.name.isEmpty() && originalValues.contains(evalVar.name)) {
            evalVar.value = originalValues[evalVar.name];
            Core()->setConfig(evalVar.name, evalVar.value);

            // update UI
            const QModelIndex valueColumnIndex =
                    siblingAtColumn(index, RizinConfigOptionsModel::ValueColumn);
            if (evalVar.type == EvaluableVarDescription::Bool) {
                const Qt::CheckState state = (evalVar.value == "true" || evalVar.value == "1")
                        ? Qt::Checked
                        : Qt::Unchecked;
                proxyModel->setData(valueColumnIndex, state, Qt::CheckStateRole);
            } else {
                proxyModel->setData(valueColumnIndex, evalVar.value, Qt::EditRole);
            }

            const auto optionChanged = getOptionChangedFromEvalVar(evalVar.name);
            if (optionChanged == OptionChanged::Asm) {
                options |= OptionChanged::Asm;
            } else if (optionChanged == OptionChanged::Debug) {
                options |= OptionChanged::Debug;
            } else if (optionChanged == OptionChanged::Analysis) {
                options |= OptionChanged::Analysis;
            } else if (optionChanged == OptionChanged::Symbols) {
                options |= OptionChanged::Symbols;
            }
        }
    }

    triggerOptionsChanged(options);

    updateItemCount();
    ui->treeView->expandAll();
}
