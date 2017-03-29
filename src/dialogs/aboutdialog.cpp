#include "aboutdialog.h"
#include "ui_aboutdialog.h"
#include "r_userconf.h"

AboutDialog::AboutDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::AboutDialog)
{
    ui->setupUi(this);
    ui->label->setText("<h1>Iaito</h1>"
                       "Version 1.0 alpha<br />"
                       "Using r2-" R2_VERSION
                       "<h2>License</h2>"
                       "This Software is released under the GNU General Public License v3.0"
                       "<h2>Authors</h2>"
                       "Hugo Teso &lt;hugo.teso@gmail.org&gt;\nSoon to be thousands more!");
}

AboutDialog::~AboutDialog()
{
    delete ui;
}

void AboutDialog::on_buttonBox_rejected()
{
    close();
}
