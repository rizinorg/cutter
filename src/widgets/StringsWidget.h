#ifndef STRINGSWIDGET_H
#define STRINGSWIDGET_H

#include "AddressableItemModel.h"
#include "CutterDescriptions.h"
#include "CutterDockWidget.h"
#include "common/StringsTask.h"

#include <QAbstractListModel>
#include <QSortFilterProxyModel>

#include <memory>

class MainWindow;
class QTreeWidgetItem;
class StringsWidget;

namespace Ui {
class StringsWidget;
}

/**
 * @brief Source model for @ref StringsWidget
 */
class StringsModel : public AddressableItemModel<QAbstractListModel>
{
    Q_OBJECT

    friend StringsWidget;

private:
    QList<StringDescription> strings;

public:
    enum Column : ut8 {
        OffsetColumn = 0,
        StringColumn,
        TypeColumn,
        LengthColumn,
        SizeColumn,
        SectionColumn,
        CommentColumn,
        ColumnCount
    };
    static const int stringDescriptionRole = Qt::UserRole;

    StringsModel(QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;

    QVariant data(const QModelIndex &index, int role) const override;
    QVariant headerData(int section, Qt::Orientation orientation,
                        int role = Qt::DisplayRole) const override;

    RVA address(const QModelIndex &index) const override;
    const StringDescription *description(const QModelIndex &index) const;
};

/**
 * @brief Proxy model for @ref StringsWidget
 */
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

/**
 * @brief Widget listing all of the strings in binary
 */
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

    void onActionCopy();

private:
    std::unique_ptr<Ui::StringsWidget> ui;

    std::shared_ptr<StringsTask> task;

    StringsModel *model;
    StringsProxyModel *proxyModel;
};

#endif // STRINGSWIDGET_H
