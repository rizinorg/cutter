#ifndef YARA_WIDGET_H
#define YARA_WIDGET_H

#include <memory>

#include <QAbstractListModel>
#include <QSortFilterProxyModel>

#include <core/Cutter.h>
#include <widgets/CutterDockWidget.h>

#include "ui_YaraWidget.h"
#include "YaraViewMenu.h"
#include "YaraSyntax.h"

class MainWindow;
class QTreeWidget;
class QTreeWidgetItem;
class YaraWidget;

namespace Ui {
class YaraWidget;
}

class YaraModel : public QAbstractListModel
{
    Q_OBJECT

    friend YaraWidget;

public:
    enum Column { OffsetColumn = 0, SizeColumn, NameColumn, ColumnCount };
    enum Role { YaraDescriptionRole = Qt::UserRole };

    YaraModel(QList<YaraDescription> *strings, QObject *parent = 0);
    virtual ~YaraModel() {};

    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const;

    int rowCount(const QModelIndex &parent = QModelIndex()) const;
    int columnCount(const QModelIndex &parent = QModelIndex()) const;

private:
    QList<YaraDescription> *strings;
};

class YaraProxyModel : public QSortFilterProxyModel
{
    Q_OBJECT

public:
    YaraProxyModel(YaraModel *sourceModel, QObject *parent = nullptr);
    virtual ~YaraProxyModel() {};

protected:
    bool filterAcceptsRow(int row, const QModelIndex &parent) const override;
    bool lessThan(const QModelIndex &left, const QModelIndex &right) const override;
};

class MetadataModel : public QAbstractListModel
{
    Q_OBJECT

    friend YaraWidget;

public:
    enum Column { NameColumn = 0, ValueColumn, ColumnCount };
    enum Role { MetadataDescriptionRole = Qt::UserRole };

    MetadataModel(QList<MetadataDescription> *metadata, QObject *parent = 0);
    virtual ~MetadataModel() {};

    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const;

    int rowCount(const QModelIndex &parent = QModelIndex()) const;
    int columnCount(const QModelIndex &parent = QModelIndex()) const;

private:
    QList<MetadataDescription> *metadata;
};

class MetadataProxyModel : public QSortFilterProxyModel
{
    Q_OBJECT

public:
    MetadataProxyModel(MetadataModel *sourceModel, QObject *parent = nullptr);
    virtual ~MetadataProxyModel() {};

protected:
    bool filterAcceptsRow(int row, const QModelIndex &parent) const override;
    bool lessThan(const QModelIndex &left, const QModelIndex &right) const override;
};

class YaraWidget : public CutterDockWidget
{
    Q_OBJECT

public:
    enum YaraViewMode { StringsMode = 0, MatchesMode, RuleMode, MetadataMode, ModeCount };
    explicit YaraWidget(MainWindow *main);
    virtual ~YaraWidget() {};

    void switchToMatches();

private slots:
    void reloadWidget();
    void onSelectedItemChanged(const QModelIndex &index);
    void showItemContextMenu(const QPoint &pt);

private:
    std::unique_ptr<Ui::YaraWidget> ui;
    std::unique_ptr<QSyntaxHighlighter> syntax;

    YaraModel *model;
    YaraProxyModel *proxyModel;
    QList<YaraDescription> strings;

    MetadataModel *metaModel;
    MetadataProxyModel *metaProxyModel;
    QList<MetadataDescription> metadata;

    YaraViewMenu *blockMenu;

    void setScrollMode();
    void refreshStrings();
    void refreshMatches();
    void refreshRule();
    void refreshMetadata();
};

#endif // YARA_WIDGET_H
