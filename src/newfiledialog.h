#ifndef NEWFILEDIALOG_H
#define NEWFILEDIALOG_H

#include <QDialog>
#include <QListWidgetItem>

namespace Ui {
class NewFileDialog;
}

class NewFileDialog : public QDialog
{
    Q_OBJECT

public:
    explicit NewFileDialog(QWidget *parent = 0);
    ~NewFileDialog();

private slots:

    void on_loadFileButton_clicked();

    void on_newFileButton_clicked();

    void on_recentsList_itemClicked(QListWidgetItem *item);

    void on_recentsList_itemDoubleClicked(QListWidgetItem *item);

    void on_cancelButton_clicked();

    void on_actionRemove_item_triggered();

    void on_createButton_clicked();

    void on_actionClear_all_triggered();

private:
    Ui::NewFileDialog *ui;

    static const int MaxRecentFiles = 5;
};

#endif // NEWFILEDIALOG_H
