#ifndef STRINGSWIDGET_H
#define STRINGSWIDGET_H

#include <memory>

#include "core/Cutter.h"
#include "CutterDockWidget.h"
#include "common/StringsTask.h"
#include "CutterTreeWidget.h"
#include "AddressableItemModel.h"

#include <QAbstractListModel>
#include <QSortFilterProxyModel>

class MainWindow;
class QTreeWidgetItem;
class StringsWidget;

namespace Ui {
class StringsWidget;
}

class StringsModel : public AddressableItemModel<QAbstractListModel>
{
    Q_OBJECT

    friend StringsWidget;

private:
    QList<StringDescription> *strings;

public:
    enum Column {
        OffsetColumn = 0,
        StringColumn,
        TypeColumn,
        LengthColumn,
        SizeColumn,
        SectionColumn,
        CommentColumn,
        ColumnCount
    };
    static const int StringDescriptionRole = Qt::UserRole;

    StringsModel(QList<StringDescription> *strings, QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;

    QVariant data(const QModelIndex &index, int role) const override;
    QVariant headerData(int section, Qt::Orientation orientation,
                        int role = Qt::DisplayRole) const override;

    RVA address(const QModelIndex &index) const override;
    const StringDescription *description(const QModelIndex &index) const;
};

class StringsProxyModel : public AddressableFilterProxyModel
{
    Q_OBJECT

public:
    StringsProxyModel(StringsModel *sourceModel, QObject *parent = nullptr);
    void setSelectedSection(QString section);

protected:
    bool filterAcceptsRow(int row, const QModelIndex &parent) const override;
    bool lessThan(const QModelIndex &left, const QModelIndex &right) const override;

    QString selectedSection;
};

class StringsWidget : public CutterDockWidget
{
    Q_OBJECT

public:
    explicit StringsWidget(MainWindow *main);
    ~StringsWidget();

private slots:
    void refreshStrings();
    void stringSearchFinished(const QList<StringDescription> &strings);
    void refreshSectionCombo();

    void on_actionCopy();

private:
    std::unique_ptr<Ui::StringsWidget> ui;

    QSharedPointer<StringsTask> task;

    StringsModel *model;
    StringsProxyModel *proxyModel;
    QList<StringDescription> strings;
    CutterTreeWidget *tree;
};

#endif // STRINGSWIDGET_H
