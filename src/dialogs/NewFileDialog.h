#ifndef NEWFILEDIALOG_H
#define NEWFILEDIALOG_H

#include <QDialog>
#include <QListWidgetItem>
#include <memory>

namespace Ui {
class NewFileDialog;
}

class MainWindow;

class NewFileDialog : public QDialog
{
    Q_OBJECT

public:
    explicit NewFileDialog(MainWindow *main);
    ~NewFileDialog();

private slots:
    void on_loadFileButton_clicked();
    void on_selectFileButton_clicked();
    void on_selectProjectFileButton_clicked();

    void on_loadProjectButton_clicked();
    void on_shellcodeButton_clicked();

    void on_aboutButton_clicked();

    void on_recentsListWidget_itemClicked(QListWidgetItem *item);
    void on_recentsListWidget_itemDoubleClicked(QListWidgetItem *item);
    void on_recentsListWidget_currentItemChanged(QListWidgetItem *current);

    void on_projectFileEdit_textChanged();
    void on_projectsListWidget_itemClicked(QListWidgetItem *item);
    void on_projectsListWidget_itemDoubleClicked(QListWidgetItem *item);

    void on_actionRemove_item_triggered();
    void on_actionClear_all_triggered();
    void on_actionRemove_project_triggered();
    void on_actionClearProjects_triggered();

    void on_tabWidget_currentChanged(int index);

protected:
    void dragEnterEvent(QDragEnterEvent *event);
    void dropEvent(QDropEvent *event);

private:
    std::unique_ptr<Ui::NewFileDialog> ui;

    MainWindow *main;

    /**
     * @return true if list is not empty
     */
    bool fillRecentFilesList();

    /**
     * @return true if list is not empty
     */
    bool fillProjectsList();
    void fillIOPluginsList();

    void updateLoadProjectButton();

    void loadFile(const QString &filename);
    void loadProject(const QString &project);
    void loadShellcode(const QString &shellcode, const int size);
    /**
     * @brief Updates IO plugin and file path based on the selected recent item
     * @param item The list item containing the IO mode and file path in its UserRole data
     */
    void updateSelectionFromItem(QListWidgetItem *item);

    bool eventFilter(QObject *obj, QEvent *event);

    static const int MaxRecentFiles = 5;
};

#endif // NEWFILEDIALOG_H
