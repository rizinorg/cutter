#ifndef TYPESWIDGET_H
#define TYPESWIDGET_H

#include "CutterDescriptions.h"
#include "CutterDockWidget.h"

#include <QAbstractListModel>
#include <QSortFilterProxyModel>

#include <memory>

class MainWindow;
class QTreeWidget;
class TypesWidget;

namespace Ui {
class TypesWidget;
}

class MainWindow;
class QTreeWidgetItem;

/**
 * @brief Source model for @ref TypesWidget
 */
class TypesModel : public QAbstractListModel
{
    Q_OBJECT

    friend TypesWidget;

private:
    QList<TypeDescription> types;

    /**
     * @brief Returns a description of the type for the given index
     */
    QVariant toolTipValue(const QModelIndex &index) const;

public:
    enum Columns : ut8 { TYPE = 0, SIZE, CATEGORY, TYPE_CLASS, FORMAT, COUNT };
    static const int typeDescriptionRole = Qt::UserRole;

    TypesModel(QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;

    QVariant data(const QModelIndex &index, int role) const override;
    QVariant headerData(int section, Qt::Orientation orientation,
                        int role = Qt::DisplayRole) const override;

    bool removeRows(int row, int count, const QModelIndex &parent = QModelIndex()) override;
};

class TypesSortFilterProxyModel : public QSortFilterProxyModel
{
    Q_OBJECT

public:
    TypesSortFilterProxyModel(TypesModel *source_model, QObject *parent = nullptr);
    void setCategory(QString category);

protected:
    bool filterAcceptsRow(int row, const QModelIndex &parent) const override;
    bool lessThan(const QModelIndex &left, const QModelIndex &right) const override;

    QString selectedCategory;
};

/**
 * @brief Widget for listing all types
 */
class TypesWidget : public CutterDockWidget
{
    Q_OBJECT

public:
    explicit TypesWidget(MainWindow *main);
    ~TypesWidget();

    /**
     * @brief Highlight and scroll to a specific type in the list
     * Clears the filter if the type is not visible in the filtered list
     * @param typeName Name of the type to select
     */
    void selectTypeByName(const QString &typeName);

private slots:
    void refreshTypes();

    /**
     * @brief Show custom context menu
     * @param pt Position of the place where the right mouse button was clicked
     */
    void showTypesContextMenu(const QPoint &pt);

    /**
     * @brief Executed on clicking the Export Types option in the context menu
     * It shows the user a file dialog box to select a file where the types
     * will be exported.
     */
    void onActionExportTypesTriggered();

    /**
     * @brief Executed on clicking the Load New types option in the context menu
     * It will open the TypesInteractionDialog where the user can either enter the
     * types manually, or can select a file from where the types will be loaded
     */
    void onActionLoadNewTypesTriggered();

    /**
     * @brief Executed on clicking the Delete Type option in the context menu
     * Upon confirmation from the user, it will delete the selected type.
     */
    void onActionDeleteTypeTriggered();

    /**
     * @brief Shows a dialog for renaming selected type
     */
    void onActionRenameTypeTriggered();

    /**
     * @brief Executed on clicking either the Edit Type or View Type options in the context menu
     * It will open the TypesInteractionDialog filled with the selected type. Depends on Edit or
     * View mode the text view would be read-only or not.
     */
    void viewType(bool readOnly = true);

    /**
     * @brief triggers when the user double-clicks an item. This will open
     * a dialog that shows the Type's content
     */
    void typeItemDoubleClicked(const QModelIndex &index);

    /**
     * @brief Opens a dialog that displays all global and local variables associated
     * with the selected type
     */
    void showUsages();

private:
    std::unique_ptr<Ui::TypesWidget> ui;

    TypesModel *typesModel;
    TypesSortFilterProxyModel *typesProxyModel;

    QAction *actionDeleteType;
    QAction *actionEditType;
    QAction *actionExportTypes;
    QAction *actionLoadNewTypes;
    QAction *actionRenameType;
    QAction *actionShowUsages;
    QAction *actionViewType;

    void setScrollMode();

    /**
     * @brief Sets the contents of the ComboBox to the supplied contents
     * @param categories The list of categories which has to be added to the ComboBox
     */
    void refreshCategoryCombo(const QStringList &categories);
};

#endif // TYPESWIDGET_H
