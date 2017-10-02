#ifndef COMMENTSDIALOG_H
#define COMMENTSDIALOG_H

#include <QDialog>
#include <memory>

namespace Ui
{
    class CommentsDialog;
}

class CommentsDialog : public QDialog
{
    Q_OBJECT

public:
    explicit CommentsDialog(QWidget *parent = 0);
    ~CommentsDialog();

    QString getComment();

private slots:
    void on_buttonBox_accepted();

    void on_buttonBox_rejected();

private:
    std::unique_ptr<Ui::CommentsDialog> ui;
};

#endif // COMMENTSDIALOG_H
