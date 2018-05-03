#ifndef NEWFILEDIALOG_H
#define NEWFILEDIALOG_H

#include <QDialog>
#include <QListWidgetItem>
#include <memory>

namespace Ui {
class NewFileDialog;
}

class NewFileDialog : public QDialog
{
    Q_OBJECT

public:
    explicit NewFileDialog(QWidget *parent = nullptr);
    ~NewFileDialog();

private slots:
    void on_loadFileButton_clicked();
    void on_selectFileButton_clicked();

    void on_selectProjectsDirButton_clicked();
    void on_loadProjectButton_clicked();

    void on_aboutButton_clicked();

    void on_recentsListWidget_itemClicked(QListWidgetItem *item);
    void on_recentsListWidget_itemDoubleClicked(QListWidgetItem *item);

    void on_projectsListWidget_itemSelectionChanged();
    void on_projectsListWidget_itemDoubleClicked(QListWidgetItem *item);

    void on_actionRemove_item_triggered();
    void on_actionClear_all_triggered();
    void on_actionRemove_project_triggered();

protected:
    void dragEnterEvent(QDragEnterEvent *event);
    void dropEvent(QDropEvent *event);

private:
    std::unique_ptr<Ui::NewFileDialog> ui;

    /*!
     * @return true if list is not empty
     */
    bool fillRecentFilesList();

    /*!
     * @return true if list is not empty
     */
    bool fillProjectsList();

    void loadFile(const QString &filename);
    void loadProject(const QString &project);

    static const int MaxRecentFiles = 5;
};

#endif // NEWFILEDIALOG_H
