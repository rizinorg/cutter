#ifndef COMMENTSDIALOG_H
#define COMMENTSDIALOG_H

#include "core/CutterCommon.h"

#include <QDialog>

#include <memory>

namespace Ui {
class CommentsDialog;
}

/**
 * @brief Dialog for adding or editing code comments at a specific address
 */
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
    void onButtonBoxAccepted();

    void onButtonBoxRejected();

private:
    std::unique_ptr<Ui::CommentsDialog> ui;

    bool eventFilter(QObject *obj, QEvent *event);
};

#endif // COMMENTSDIALOG_H
