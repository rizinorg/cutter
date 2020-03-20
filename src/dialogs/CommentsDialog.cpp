#include "CommentsDialog.h"
#include "ui_CommentsDialog.h"

#include "core/Cutter.h"

CommentsDialog::CommentsDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::CommentsDialog)
{
    ui->setupUi(this);
    setWindowFlags(windowFlags() & (~Qt::WindowContextHelpButtonHint));

    // Event filter for capturing Ctrl/Cmd+Return
    ui->textEdit->installEventFilter(this);
}

CommentsDialog::~CommentsDialog() {}

void CommentsDialog::on_buttonBox_accepted()
{
}

void CommentsDialog::on_buttonBox_rejected()
{
    close();
}

QString CommentsDialog::getComment()
{
    QString ret = ui->textEdit->document()->toPlainText();
    return ret;
}

void CommentsDialog::setComment(const QString &comment)
{
    ui->textEdit->document()->setPlainText(comment);
}

void CommentsDialog::addOrEditComment(RVA offset, QWidget *parent)
{
    QString oldComment = Core()->cmdRawAt("CC.", offset);
    // Remove newline at the end added by cmd
    oldComment.remove(oldComment.length() - 1, 1);
    CommentsDialog c(parent);

    if (oldComment.isNull() || oldComment.isEmpty()) {
        c.setWindowTitle(tr("Add Comment at %1").arg(RAddressString(offset)));
    } else {
        c.setWindowTitle(tr("Edit Comment at %1").arg(RAddressString(offset)));
    }

    c.setComment(oldComment);
    if (c.exec()) {
        QString comment = c.getComment();
        if (comment.isEmpty()) {
            Core()->delComment(offset);
        } else {
            Core()->setComment(offset, comment);
        }
    }
}

bool CommentsDialog::eventFilter(QObject */*obj*/, QEvent *event)
{
    if (event -> type() == QEvent::KeyPress) {
        QKeyEvent *keyEvent = static_cast <QKeyEvent *> (event);

        // Confirm comment by pressing Ctrl/Cmd+Return
        if ((keyEvent -> modifiers() & Qt::ControlModifier) &&
                ((keyEvent -> key() == Qt::Key_Enter) || (keyEvent -> key() == Qt::Key_Return))) {
            this->accept();
            return true;
        }
    }

    return false;
}
