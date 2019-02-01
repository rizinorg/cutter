#ifndef CLASSESWIDGET_H
#define CLASSESWIDGET_H

#include <memory>

#include "Cutter.h"
#include "CutterDockWidget.h"

#include <QAbstractListModel>
#include <QSortFilterProxyModel>

namespace Ui {
class ClassesWidget;
}

class QTreeWidget;
class QTreeWidgetItem;
class MainWindow;
class ClassesWidget;

class ClassesModel: public QAbstractItemModel
{
public:
    enum Columns { NAME = 0, TYPE, OFFSET, VTABLE, COUNT };
    enum class RowType { Class = 0, Method = 1, Field = 2, Base = 3, VTable = 4 };

    static const int OffsetRole = Qt::UserRole;
    static const int NameRole = Qt::UserRole + 1;
    static const int TypeRole = Qt::UserRole + 2;
    static const int VTableOffsetRole = Qt::UserRole + 3;
    static const int DataRole = Qt::UserRole + 4;

    explicit ClassesModel(QObject *parent = nullptr) : QAbstractItemModel(parent) {}

    QVariant headerData(int section, Qt::Orientation orientation,
                        int role = Qt::DisplayRole) const override;
};

Q_DECLARE_METATYPE(ClassesModel::RowType)

class BinClassesModel: public ClassesModel
{
    Q_OBJECT

private:
    QList<BinClassDescription> classes;

    QModelIndex index(int row, int column, const QModelIndex &parent = QModelIndex()) const override;

    QModelIndex parent(const QModelIndex &index) const override;
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;

    int columnCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role) const override;

public:
    explicit BinClassesModel(QObject *parent = nullptr);
    void setClasses(const QList<BinClassDescription> &classes);
};


class AnalClassesModel: public ClassesModel
{
Q_OBJECT

private:
    struct Attribute
    {
        enum class Type { VTable, Base, Method };
        Type type;
        QVariant data;

        Attribute() = default;
        Attribute(Type type, const QVariant &data) : type(type), data(data) {}
    };

    QList<QString> classes;
    std::unique_ptr<QMap<QString, QVector<Attribute>>> attrs;

    const QVector<Attribute> &getAttrs(const QString &cls) const;

    QModelIndex index(int row, int column, const QModelIndex &parent = QModelIndex()) const override;

    QModelIndex parent(const QModelIndex &index) const override;
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    bool hasChildren(const QModelIndex &parent = QModelIndex()) const override;

    int columnCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role) const override;

public:
    explicit AnalClassesModel(QObject *parent = nullptr);

    void refreshClasses();
};



class ClassesSortFilterProxyModel : public QSortFilterProxyModel
{
    Q_OBJECT

public:
    explicit ClassesSortFilterProxyModel(QObject *parent = nullptr);

protected:
    bool filterAcceptsRow(int row, const QModelIndex &parent) const override;
    bool lessThan(const QModelIndex &left, const QModelIndex &right) const override;
};



class ClassesWidget : public CutterDockWidget
{
    Q_OBJECT

public:
    explicit ClassesWidget(MainWindow *main, QAction *action = nullptr);
    ~ClassesWidget();

private slots:
    void on_classesTreeView_doubleClicked(const QModelIndex &index);

    void on_seekToVTableAction_triggered();
    void on_addMethodAction_triggered();
    void on_editMethodAction_triggered();

    void showContextMenu(const QPoint &pt);

    void refreshClasses();

private:
    enum class Source { BIN, ANAL };

    Source getSource();

    std::unique_ptr<Ui::ClassesWidget> ui;

    BinClassesModel *bin_model = nullptr;
    AnalClassesModel *anal_model = nullptr;
    ClassesSortFilterProxyModel *proxy_model;
};


#endif // CLASSESWIDGET_H
