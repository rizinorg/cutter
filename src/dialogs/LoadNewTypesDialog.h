#ifndef LOADNEWTYPESDIALOG_H
#define LOADNEWTYPESDIALOG_H

#include <QDialog>
#include <memory>

namespace Ui {
class LoadNewTypesDialog;
}

class LoadNewTypesDialog : public QDialog
{
    Q_OBJECT

public:
    explicit LoadNewTypesDialog(QWidget *parent = nullptr);
    ~LoadNewTypesDialog();

private slots:
    void on_selectFileButton_clicked();
    void on_plainTextEdit_textChanged();
    void done(int r);

private:
    //std::unique_ptr<Ui::LoadNewTypesDialog> ui;
    Ui::LoadNewTypesDialog *ui;
signals:
    /*!
     * \brief Emitted when new types are loaded
     */
    void newTypesLoaded();
};

#endif // LOADNEWTYPESDIALOG_H
