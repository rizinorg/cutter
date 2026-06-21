#ifndef NEWFILEDIALOG_H
#define NEWFILEDIALOG_H

#include <QDialog>
#include <QListWidgetItem>

#include <memory>

namespace Ui {
class NewFileDialog;
}

class MainWindow;

/**
 * @brief Dialog for opening a new file or project
 */
class NewFileDialog : public QDialog
{
    Q_OBJECT

public:
    explicit NewFileDialog(MainWindow *main);
    ~NewFileDialog();

private slots:
    void onLoadFileButtonClicked();
    void onSelectFileButtonClicked();
    void onSelectProjectFileButtonClicked();

    void onLoadProjectButtonClicked();
    void onShellcodeButtonClicked();

    void onAboutButtonClicked();

    void onRecentsListWidgetItemClicked(QListWidgetItem *item);
    void onRecentsListWidgetItemDoubleClicked(QListWidgetItem *item);
    void onRecentsListWidgetCurrentItemChanged(QListWidgetItem *current);

    void onProjectFileEditTextChanged();
    void onProjectsListWidgetItemClicked(QListWidgetItem *item);
    void onProjectsListWidgetItemDoubleClicked(QListWidgetItem *item);

    void onActionRemoveItemTriggered();
    void onActionClearAllTriggered();
    void onActionRemoveProjectTriggered();
    void onActionClearProjectsTriggered();

    void onTabWidgetCurrentChanged(int index);

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

    static const int maxRecentFiles = 5;
};

#endif // NEWFILEDIALOG_H
