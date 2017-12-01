#include "CommentsDialog.h"
#include "ui_CommentsDialog.h"

CommentsDialog::CommentsDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::CommentsDialog)
{
    ui->setupUi(this);
    setWindowFlags(windowFlags() & (~Qt::WindowContextHelpButtonHint));
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
    QString ret = ui->lineEdit->text();
    return ret;
}

void CommentsDialog::setComment(const QString &comment)
{
    ui->lineEdit->setText(comment);
}