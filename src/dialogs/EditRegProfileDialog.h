#ifndef EDITREGPROFILEDIALOG_H
#define EDITREGPROFILEDIALOG_H

#include <QDialog>
#include <QStandardItemModel>
#include <QTableView>
#include <memory>

namespace Ui {
class EditRegProfileDialog;
}

/**
 * @brief Dialog for editing Rizin register profiles using tables or raw text
 */
class EditRegProfileDialog : public QDialog
{
    Q_OBJECT

public:
    explicit EditRegProfileDialog(QWidget *parent = nullptr);
    ~EditRegProfileDialog();

    /**
     * @brief Loads a raw profile string into the editor
     * @param data The raw Rizin profile string to parse
     */
    void setProfileData(const QString &data);

    /**
     * @brief Retrieves the current profile data from the active editor tab
     * @return The formatted profile string
     */
    QString getProfleData() const;

private slots:
    /**
     * @brief Adds a new row under the cursor, or at the end if no row is selected
     */
    void onAddRow();

    /**
     * @brief Deletes the currently selected row
     */
    void onRemoveRow();

    /**
     * @brief Syncs data between tables and the raw text editor when switching tabs
     * @param index The index of the newly selected tab
     */
    void onTabChanged(int index);

    /**
     * @brief Shows a custom context menu for adding or removing rows at the clicked position
     * @param pos The position where the context menu was requested, relative to the table viewport
     */
    void showContextMenu(const QPoint &pos);

private:
    enum CustomRole { CommentRole = Qt::UserRole + 1 };
    enum TabIndex { TAB_ALIASES, TAB_REGISTERS, TAB_RAW };
    enum AliasCol { ALIAS_NAME = 0, ALIAS_REG, ALIAS_MAX_COL };
    enum RegCol {
        REG_TYPE = 0,
        REG_NAME,
        REG_SIZE,
        REG_OFFSET,
        REG_PACKED,
        REG_FLAGS,
        REG_MAX_COL,
    };

    struct TabContext
    {
        QTableView *view;
        QStandardItemModel *model;
    };

    QStandardItemModel *aliasModel;
    QStandardItemModel *regModel;

    /**
     * @brief Detects which tab is active
     * @return A TabContext containing the active view and model
     */
    TabContext getActiveTab() const;

    /**
     * @brief Converts raw profile string to table entries
     * @param data The string to be parsed into table enries
     */
    void parseToTables(const QString &data);

    /**
     * @brief Converts table entries to raw profile string
     * @return A tab-separated string representation of the tables
     */
    QString tablesToText() const;

    std::unique_ptr<Ui::EditRegProfileDialog> ui;
};

#endif
