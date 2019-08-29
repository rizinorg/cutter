#ifndef COMMENTSDIALOG_H
#define COMMENTSDIALOG_H

#include <QDialog>
#include <memory>

#include "core/CutterCommon.h"

namespace Ui {
class CommentsDialog;
}

class CommentsDialog : public QDialog
{
    Q_OBJECT

public:
    explicit CommentsDialog(QWidget *parent = nullptr);
    ~CommentsDialog();

    QString getComment();
    void setComment(const QString &comment);

    static void addOrEditComment(RVA offset, QWidget *parent);
private slots:
    void on_buttonBox_accepted();

    void on_buttonBox_rejected();

private:
    std::unique_ptr<Ui::CommentsDialog> ui;

    bool eventFilter(QObject *obj, QEvent *event);
};

#endif // COMMENTSDIALOG_H
