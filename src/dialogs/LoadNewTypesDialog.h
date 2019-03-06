#ifndef LOADNEWTYPESDIALOG_H
#define LOADNEWTYPESDIALOG_H

#include <QDialog>
#include <memory>

namespace Ui {
class LoadNewTypesDialog;
}
class SyntaxHighlighter;

class LoadNewTypesDialog : public QDialog
{
    Q_OBJECT

public:
    explicit LoadNewTypesDialog(QWidget *parent = nullptr);
    ~LoadNewTypesDialog();

private slots:
    /**
     * @brief Executed when the user clicks the selectFileButton
     * Opens a File Dialog from where the user can select a file from where
     * the types will be loaded.
     */
    void on_selectFileButton_clicked();

    /**
     * @brief Executed whenever the text inside the textbox changes
     * When the text box is empty, the OK button is disabled.
     */
    void on_plainTextEdit_textChanged();

    /**
     * @brief done Closes the dialog and sets its result code to r
     * @param r The value which will be returned by exec()
     */
    void done(int r) override;

private:
    std::unique_ptr<Ui::LoadNewTypesDialog> ui;
    SyntaxHighlighter *syntaxHighLighter;

signals:
    /**
     * @brief Emitted when new types are loaded
     */
    void newTypesLoaded();
};

#endif // LOADNEWTYPESDIALOG_H
