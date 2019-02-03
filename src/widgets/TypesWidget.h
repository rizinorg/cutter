#ifndef TYPESWIDGET_H
#define TYPESWIDGET_H

#include <memory>

#include "Cutter.h"
#include "CutterDockWidget.h"
#include "CutterTreeWidget.h"

#include <QAbstractListModel>
#include <QSortFilterProxyModel>

class MainWindow;
class QTreeWidget;
class TypesWidget;

namespace Ui {
class TypesWidget;
}


class MainWindow;
class QTreeWidgetItem;


class TypesModel: public QAbstractListModel
{
    Q_OBJECT

    friend TypesWidget;

private:
    QList<TypeDescription> *types;

public:
    enum Columns { TYPE = 0, SIZE, FORMAT, CATEGORY, COUNT };
    static const int TypeDescriptionRole = Qt::UserRole;

    TypesModel(QList<TypeDescription> *types, QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = QModelIndex()) const;
    int columnCount(const QModelIndex &parent = QModelIndex()) const;

    QVariant data(const QModelIndex &index, int role) const;
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const;
};



class TypesSortFilterProxyModel : public QSortFilterProxyModel
{
    Q_OBJECT

    friend TypesWidget;

public:
    TypesSortFilterProxyModel(TypesModel *source_model, QObject *parent = nullptr);

protected:
    bool filterAcceptsRow(int row, const QModelIndex &parent) const override;
    bool lessThan(const QModelIndex &left, const QModelIndex &right) const override;

    QString selectedCategory;
};



class TypesWidget : public CutterDockWidget
{
    Q_OBJECT

public:
    explicit TypesWidget(MainWindow *main, QAction *action = nullptr);
    ~TypesWidget();

private slots:
    void refreshTypes();

    /*!
     * \brief Show custom context menu
     * \param pt Position of the place where the right mouse button was clicked
     */
    void showTypesContextMenu(const QPoint &pt);

    /*!
     * \brief Executed on clicking the Export Types option in the context menu
     */
    void on_actionExport_Types_triggered();

    /*!
     * \brief Executed on clicking the Load New types option in the context menu
     */
    void on_actionLoad_New_Types_triggered();

private:
    std::unique_ptr<Ui::TypesWidget> ui;

    TypesModel *types_model;
    TypesSortFilterProxyModel *types_proxy_model;
    QList<TypeDescription> types;
    CutterTreeWidget *tree;

    void setScrollMode();

    /*!
     * \brief Sets the contents of the ComboBox to the supplied contents
     * \param categories The list of categories which has to be added to the ComboBox
     */
    void refreshCategoryCombo(const QStringList &categories);
};


#endif // TYPESWIDGET_H
