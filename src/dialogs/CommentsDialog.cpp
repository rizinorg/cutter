#include "CommentsDialog.h"
#include "ui_CommentsDialog.h"

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
