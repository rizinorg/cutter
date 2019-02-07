#include "RenameDialog.h"
#include "ui_RenameDialog.h"

RenameDialog::RenameDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::RenameDialog)
{
    ui->setupUi(this);
    setWindowFlags(windowFlags() & (~Qt::WindowContextHelpButtonHint));
}

RenameDialog::~RenameDialog() {}

void RenameDialog::on_buttonBox_accepted()
{
    // Rename function and refresh widgets
    QString name = ui->nameEdit->text();
}

void RenameDialog::on_buttonBox_rejected()
{
    close();
}

void RenameDialog::setName(QString fcnName)
{
    ui->nameEdit->setText(fcnName);
}

QString RenameDialog::getName() const
{
    return ui->nameEdit->text();
}

void RenameDialog::setPlaceholderText(const QString &text)
{
    ui->nameEdit->setPlaceholderText(text);
}

bool RenameDialog::showDialog(const QString &title, QString *name, const QString &placeholder, QWidget *parent)
{
    RenameDialog dialog(parent);
    dialog.setWindowTitle(title);
    dialog.setPlaceholderText(placeholder);
    dialog.setName(*name);
    int result = dialog.exec();
    *name = dialog.getName();
    return result == QDialog::DialogCode::Accepted;
}
