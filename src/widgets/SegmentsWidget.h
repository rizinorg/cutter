#ifndef SEGMENTSWIDGET_H
#define SEGMENTSWIDGET_H

#include "CutterDescriptions.h"
#include "widgets/ListDockWidget.h"

#include <QAbstractListModel>
#include <QSortFilterProxyModel>

class QAbstractItemView;
class SegmentsWidget;

/**
 * @brief Source model for @ref SegmentsWidget
 */
class SegmentsModel : public AddressableItemModel<QAbstractListModel>
{
    Q_OBJECT

    friend SegmentsWidget;

private:
    QList<SegmentDescription> segments;

public:
    enum Column : ut8 {
        NameColumn = 0,
        SizeColumn,
        AddressColumn,
        EndAddressColumn,
        PermColumn,
        CommentColumn,
        ColumnCount
    };
    enum Role : ut16 { SegmentDescriptionRole = Qt::UserRole };

    SegmentsModel(QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;

    QVariant data(const QModelIndex &index, int role) const override;
    QVariant headerData(int segment, Qt::Orientation orientation,
                        int role = Qt::DisplayRole) const override;

    RVA address(const QModelIndex &index) const override;
    QString name(const QModelIndex &index) const override;
};

/**
 * @brief Proxy model for @ref SegmentsWidget
 */
class SegmentsProxyModel : public AddressableFilterProxyModel
{
    Q_OBJECT

public:
    SegmentsProxyModel(SegmentsModel *sourceModel, QObject *parent = nullptr);

protected:
    bool lessThan(const QModelIndex &left, const QModelIndex &right) const override;
};

/**
 * @brief Widget showing list of all segments in a binary
 */
class SegmentsWidget : public ListDockWidget
{
    Q_OBJECT

public:
    explicit SegmentsWidget(MainWindow *main);
    ~SegmentsWidget();

private slots:
    void refreshSegments();

private:
    SegmentsModel *segmentsModel;
    SegmentsProxyModel *proxyModel;
};

#endif // SEGMENTSWIDGET_H
