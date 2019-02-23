#ifndef STRINGSWIDGET_H
#define STRINGSWIDGET_H

#include <memory>

#include "core/Cutter.h"
#include "CutterDockWidget.h"
#include "common/StringsTask.h"
#include "CutterTreeWidget.h"

#include <QAbstractListModel>
#include <QSortFilterProxyModel>

class MainWindow;
class QTreeWidgetItem;
class StringsWidget;

namespace Ui {
class StringsWidget;
}

class StringsModel: public QAbstractListModel
{
    Q_OBJECT

    friend StringsWidget;

private:
    QList<StringDescription> *strings;

public:
    enum Column { OffsetColumn = 0, StringColumn, TypeColumn, LengthColumn, SizeColumn, SectionColumn, ColumnCount };
    static const int StringDescriptionRole = Qt::UserRole;

    StringsModel(QList<StringDescription> *strings, QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = QModelIndex()) const;
    int columnCount(const QModelIndex &parent = QModelIndex()) const;

    QVariant data(const QModelIndex &index, int role) const;
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const;
};



class StringsProxyModel : public QSortFilterProxyModel
{
    Q_OBJECT

    friend StringsWidget;

public:
    StringsProxyModel(StringsModel *sourceModel, QObject *parent = nullptr);

protected:
    bool filterAcceptsRow(int row, const QModelIndex &parent) const override;
    bool lessThan(const QModelIndex &left, const QModelIndex &right) const override;

    QString selectedSection;
};


class StringsWidget : public CutterDockWidget
{
    Q_OBJECT

public:
    explicit StringsWidget(MainWindow *main, QAction *action = nullptr);
    ~StringsWidget();

private slots:
    void on_stringsTreeView_doubleClicked(const QModelIndex &index);

    void refreshStrings();
    void stringSearchFinished(const QList<StringDescription> &strings);
    void refreshSectionCombo();

    void showStringsContextMenu(const QPoint &pt);
    void on_actionX_refs_triggered();
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
