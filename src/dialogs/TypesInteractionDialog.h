#ifndef TYPESINTERACTIONDIALOG_H
#define TYPESINTERACTIONDIALOG_H

#include <QDialog>
#include <memory>

namespace Ui {
class TypesInteractionDialog;
}

class QSyntaxHighlighter;

class TypesInteractionDialog : public QDialog
{
    Q_OBJECT

public:
    explicit TypesInteractionDialog(QWidget *parent = nullptr, bool readOnly = false);
    ~TypesInteractionDialog();
    /**
     * @brief Fill the Dialog's TextEdit object with the content
     * passed to the function. The content will most likely be a C
     * representation of a Type.
     * @param content - The content which should be in the TextEdit object.
     * most likely will be a C representation of a Type.
     * @param readonly - Will be set as "true" for viewing mode
     */
    void fillTextArea(QString content);

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
    std::unique_ptr<Ui::TypesInteractionDialog> ui;
    QSyntaxHighlighter *syntaxHighLighter;

signals:
    /**
     * @brief Emitted when new types are loaded
     */
    void newTypesLoaded();
};

#endif // TYPESINTERACTIONDIALOG_H
