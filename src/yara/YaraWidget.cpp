#include "YaraWidget.h"

#include <core/MainWindow.h>
#include <common/Helpers.h>

#include <QJsonArray>
#include <QJsonObject>

YaraModel::YaraModel(QList<YaraDescription> *strings, QObject *parent)
    : QAbstractListModel(parent), strings(strings)
{
}

int YaraModel::rowCount(const QModelIndex &) const
{
    return strings->count();
}

int YaraModel::columnCount(const QModelIndex &) const
{
    return YaraModel::ColumnCount;
}

QVariant YaraModel::data(const QModelIndex &index, int role) const
{
    if (index.row() >= strings->count())
        return QVariant();

    const YaraDescription &desc = strings->at(index.row());

    switch (role) {
    case Qt::DisplayRole:
        switch (index.column()) {
        case OffsetColumn:
            return RzAddressString(desc.offset);
        case SizeColumn:
            return RzSizeString(desc.size);
        case NameColumn:
            return desc.name;
        default:
            return QVariant();
        }

    case YaraDescriptionRole:
        return QVariant::fromValue(desc);

    case Qt::ToolTipRole: {
        return desc.name;
    }

    default:
        return QVariant();
    }
}

QVariant YaraModel::headerData(int section, Qt::Orientation, int role) const
{
    switch (role) {
    case Qt::DisplayRole:
        switch (section) {
        case OffsetColumn:
            return tr("Offset");
        case SizeColumn:
            return tr("Size");
        case NameColumn:
            return tr("Name");
        default:
            return QVariant();
        }
    default:
        return QVariant();
    }
}

YaraProxyModel::YaraProxyModel(YaraModel *sourceModel, QObject *parent)
    : QSortFilterProxyModel(parent)
{
    setSourceModel(sourceModel);
}

bool YaraProxyModel::filterAcceptsRow(int row, const QModelIndex &parent) const
{
    QModelIndex index = sourceModel()->index(row, 0, parent);
    YaraDescription entry = index.data(YaraModel::YaraDescriptionRole).value<YaraDescription>();
    return qhelpers::filterStringContains(entry.name, this);
}

bool YaraProxyModel::lessThan(const QModelIndex &left, const QModelIndex &right) const
{
    YaraDescription leftEntry = left.data(YaraModel::YaraDescriptionRole).value<YaraDescription>();
    YaraDescription rightEntry =
            right.data(YaraModel::YaraDescriptionRole).value<YaraDescription>();

    switch (left.column()) {
    case YaraModel::OffsetColumn:
        return leftEntry.offset < rightEntry.offset;
    case YaraModel::SizeColumn:
        return leftEntry.size < rightEntry.size;
    case YaraModel::NameColumn:
        return leftEntry.name < rightEntry.name;
    default:
        break;
    }

    return leftEntry.name < rightEntry.name;
}

MetadataModel::MetadataModel(QList<MetadataDescription> *metadata, QObject *parent)
    : QAbstractListModel(parent), metadata(metadata)
{
}

int MetadataModel::rowCount(const QModelIndex &) const
{
    return metadata->count();
}

int MetadataModel::columnCount(const QModelIndex &) const
{
    return MetadataModel::ColumnCount;
}

QVariant MetadataModel::data(const QModelIndex &index, int role) const
{
    if (index.row() >= metadata->count())
        return QVariant();

    const MetadataDescription &desc = metadata->at(index.row());

    switch (role) {
    case Qt::DisplayRole:
        switch (index.column()) {
        case ValueColumn:
            return desc.value;
        case NameColumn:
            return desc.name;
        default:
            return QVariant();
        }

    case MetadataDescriptionRole:
        return QVariant::fromValue(desc);

    case Qt::ToolTipRole: {
        return desc.name;
    }

    default:
        return QVariant();
    }
}

QVariant MetadataModel::headerData(int section, Qt::Orientation, int role) const
{
    switch (role) {
    case Qt::DisplayRole:
        switch (section) {
        case NameColumn:
            return tr("Name");
        case ValueColumn:
            return tr("Value");
        default:
            return QVariant();
        }
    default:
        return QVariant();
    }
}

MetadataProxyModel::MetadataProxyModel(MetadataModel *sourceModel, QObject *parent)
    : QSortFilterProxyModel(parent)
{
    setSourceModel(sourceModel);
}

bool MetadataProxyModel::filterAcceptsRow(int row, const QModelIndex &parent) const
{
    QModelIndex index = sourceModel()->index(row, 0, parent);
    MetadataDescription entry =
            index.data(MetadataModel::MetadataDescriptionRole).value<MetadataDescription>();
    return qhelpers::filterStringContains(entry.name, this);
}

bool MetadataProxyModel::lessThan(const QModelIndex &left, const QModelIndex &right) const
{
    MetadataDescription leftEntry =
            left.data(MetadataModel::MetadataDescriptionRole).value<MetadataDescription>();
    MetadataDescription rightEntry =
            right.data(MetadataModel::MetadataDescriptionRole).value<MetadataDescription>();

    switch (left.column()) {
    case MetadataModel::NameColumn:
        return leftEntry.name < rightEntry.name;
    case MetadataModel::ValueColumn:
        return leftEntry.value < rightEntry.value;
    default:
        break;
    }

    return leftEntry.name < rightEntry.name;
}

YaraWidget::YaraWidget(MainWindow *main)
    : CutterDockWidget(main), ui(new Ui::YaraWidget), blockMenu(new YaraViewMenu(this, mainWindow))
{
    ui->setupUi(this);

    metaModel = new MetadataModel(&metadata, this);
    metaProxyModel = new MetadataProxyModel(metaModel, this);

    model = new YaraModel(&strings, this);
    proxyModel = new YaraProxyModel(model, this);
    ui->yaraTreeView->setModel(proxyModel);
    ui->yaraTreeView->sortByColumn(YaraModel::OffsetColumn, Qt::AscendingOrder);

    ui->yaraRuleEditor->setVisible(false);
    ui->yaraRuleEditor->setTabStopDistance(40);
    this->syntax.reset(new YaraSyntax(ui->yaraRuleEditor->document()));

    ui->yaraSelector->addItem(tr("Strings"), "");
    ui->yaraSelector->addItem(tr("Matches"), "");
    ui->yaraSelector->addItem(tr("Rule"), "");
    ui->yaraSelector->addItem(tr("Metadata"), "");
    ui->yaraSelector->setCurrentIndex(StringsMode);

    setScrollMode();

    this->connect(this, &QWidget::customContextMenuRequested, this,
                  &YaraWidget::showItemContextMenu);
    this->setContextMenuPolicy(Qt::CustomContextMenu);

    this->connect(ui->yaraTreeView->selectionModel(), &QItemSelectionModel::currentChanged, this,
                  &YaraWidget::onSelectedItemChanged);

    this->connect(Core(), &CutterCore::refreshAll, this, &YaraWidget::reloadWidget);
    this->connect(Core(), &CutterCore::yaraStringsChanged, this, &YaraWidget::reloadWidget);

    this->connect(ui->yaraSelector,
                  static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged), this,
                  &YaraWidget::reloadWidget);

    this->addActions(this->blockMenu->actions());
}

void YaraWidget::reloadWidget()
{
    int index = ui->yaraSelector->currentIndex();
    switch (index) {
    default:
        ui->yaraSelector->setCurrentIndex(StringsMode);
        /* fallthru */
    case YaraViewMode::StringsMode:
        ui->yaraTreeView->setVisible(true);
        ui->yaraRuleEditor->setVisible(false);
        ui->yaraTreeView->setModel(proxyModel);
        ui->yaraTreeView->sortByColumn(YaraModel::OffsetColumn, Qt::AscendingOrder);
        refreshStrings();
        break;
    case YaraViewMode::MatchesMode:
        ui->yaraTreeView->setVisible(true);
        ui->yaraRuleEditor->setVisible(false);
        ui->yaraTreeView->setModel(proxyModel);
        ui->yaraTreeView->sortByColumn(YaraModel::OffsetColumn, Qt::AscendingOrder);
        refreshMatches();
        break;
    case YaraViewMode::RuleMode:
        ui->yaraTreeView->setVisible(false);
        ui->yaraRuleEditor->setVisible(true);
        refreshRule();
        break;
    case YaraViewMode::MetadataMode:
        ui->yaraTreeView->setVisible(true);
        ui->yaraRuleEditor->setVisible(false);
        ui->yaraTreeView->setModel(metaProxyModel);
        ui->yaraTreeView->sortByColumn(MetadataModel::NameColumn, Qt::AscendingOrder);
        refreshMetadata();
        break;
    }
}

static inline QList<YaraDescription> toYaraDescriptionList(QJsonArray &array)
{
    QList<YaraDescription> list;

    for (const QJsonValue &value : array) {
        YaraDescription desc;
        QJsonObject obj = value.toObject();

        desc.offset = obj["offset"].toVariant().toULongLong();
        desc.size = obj["size"].toVariant().toULongLong();
        desc.name = obj["name"].toString();

        list << desc;
    }

    return list;
}

void YaraWidget::refreshStrings()
{
    model->beginResetModel();
    QJsonArray array = Core()->cmdj("yarasj").array();
    strings = toYaraDescriptionList(array);
    model->endResetModel();

    ui->yaraTreeView->resizeColumnToContents(0);
    ui->yaraTreeView->resizeColumnToContents(1);
    ui->yaraTreeView->resizeColumnToContents(2);
}

void YaraWidget::refreshMatches()
{
    model->beginResetModel();
    QJsonArray array = Core()->cmdj("yaraMj").array();
    strings = toYaraDescriptionList(array);
    model->endResetModel();

    ui->yaraTreeView->resizeColumnToContents(0);
    ui->yaraTreeView->resizeColumnToContents(1);
    ui->yaraTreeView->resizeColumnToContents(2);
}

void YaraWidget::refreshRule()
{
    QString rule = Core()->cmd("yarac placeholder_name");
    ui->yaraRuleEditor->setPlainText(rule);
    syntax->rehighlight();
}

void YaraWidget::refreshMetadata()
{
    metaModel->beginResetModel();
    metadata.clear();
    QJsonObject json = Core()->cmdj("yaramj").object();

    foreach (const QString &key, json.keys()) {
        MetadataDescription desc;
        desc.name = key;
        desc.value = YaraAddMetaDialog::isKeyword(key) ? tr("Autofill") : json[key].toString();
        metadata << desc;
    }
    metaModel->endResetModel();

    ui->yaraTreeView->resizeColumnToContents(0);
    ui->yaraTreeView->resizeColumnToContents(1);
}

void YaraWidget::switchToMatches()
{
    ui->yaraSelector->setCurrentIndex(MatchesMode);
}

void YaraWidget::setScrollMode()
{
    qhelpers::setVerticalScrollMode(ui->yaraTreeView);
}

void YaraWidget::onSelectedItemChanged(const QModelIndex &index)
{
    int mode = ui->yaraSelector->currentIndex();
    if (index.isValid()) {
        if (mode == YaraViewMode::MetadataMode) {
            const MetadataDescription &entry = metadata.at(index.row());
            blockMenu->setMetaTarget(entry);
        } else {
            const YaraDescription &entry = strings.at(index.row());
            blockMenu->setYaraTarget(entry, mode == YaraViewMode::StringsMode);
        }
    } else {
        blockMenu->clearTarget();
        if (mode == YaraViewMode::MetadataMode) {
            MetadataDescription entry;
            blockMenu->setMetaTarget(entry);
        } else {
            YaraDescription entry;
            blockMenu->setYaraTarget(entry, mode == YaraViewMode::StringsMode);
        }
    }
}

void YaraWidget::showItemContextMenu(const QPoint &pt)
{
    QModelIndex position;
    int index = ui->yaraSelector->currentIndex();
    switch (index) {
    case YaraViewMode::StringsMode:
    case YaraViewMode::MatchesMode:
        position = ui->yaraTreeView->currentIndex();
        if (position.isValid()) {
            const YaraDescription &entry = strings.at(position.row());
            blockMenu->setYaraTarget(entry, index == YaraViewMode::StringsMode);
        } else {
            YaraDescription entry;
            blockMenu->setYaraTarget(entry, index == YaraViewMode::StringsMode);
        }
        blockMenu->exec(this->mapToGlobal(pt));
        break;
    case YaraViewMode::RuleMode:
        break;
    case YaraViewMode::MetadataMode:
        position = ui->yaraTreeView->currentIndex();
        if (position.isValid()) {
            const MetadataDescription &entry = metadata.at(position.row());
            blockMenu->setMetaTarget(entry);
        } else {
            MetadataDescription entry;
            blockMenu->setMetaTarget(entry);
        }
        blockMenu->exec(this->mapToGlobal(pt));
        break;
    default:
        break;
    }
}