#include "RenameDialog.h"
#include "ui_RenameDialog.h"

#include <QMessageBox>
#include <QtDebug>

RenameDialog::RenameDialog(QWidget *parent, Type type) :
    QDialog(parent),
    ui(new Ui::RenameDialog)
{
    ui->setupUi(this);
    this->type = type;
    setWindowFlags(windowFlags() & (~Qt::WindowContextHelpButtonHint));
    connect(ui->autoRenameCheckBox, SIGNAL(stateChanged(int)), this, SLOT(autoRenameStateChanged(int)));
    connect(ui->realnameLineEdit, SIGNAL(textEdited(QString)), this, SLOT(realnameTextEdited(QString)));
}

RenameDialog::~RenameDialog() {}

void RenameDialog::on_buttonBox_accepted()
{
    // Rename function and refresh widgets
    QString name = ui->nameLineEdit->text();
}

void RenameDialog::on_buttonBox_rejected()
{
    close();
}

void RenameDialog::setName(QString name)
{
    ui->nameLineEdit->setText(name);
}

QString RenameDialog::getName() const
{
    return ui->nameLineEdit->text();
}

void RenameDialog::setRealName(QString name)
{
    ui->realnameLineEdit->setText(name);
    ui->realnameLineEdit->selectAll();
}

QString RenameDialog::getRealName() const
{
    return ui->realnameLineEdit->text();
}

void RenameDialog::setDialogType(Type type)
{
    this->type = type;
}

void RenameDialog::setPlaceholderText(const QString &text)
{
    ui->realnameLineEdit->setPlaceholderText(text);
}

bool RenameDialog::showDialog(const QString &title, QString *name, const QString &placeholder,
                              Type type, QWidget *parent)
{
    RenameDialog dialog(parent);

    dialog.type = type;
    dialog.setWindowTitle(title);
    dialog.setPlaceholderText(placeholder);
    dialog.setName(*name);
    int result = dialog.exec();
    *name = dialog.getName();
    return result == QDialog::DialogCode::Accepted;
}


void RenameDialog::autoRenameStateChanged(int state)
{
       switch (state) {
       case Qt::Checked:
           ui->nameLineEdit->setEnabled(false);
           break;
       case Qt::Unchecked:
           ui->nameLineEdit->setEnabled(true);
           break;
       }
}

void RenameDialog::realnameTextEdited(QString text)
{
    if (ui->autoRenameCheckBox->isChecked()) {
        // Don't duplicate flag prefixes
        if (text.startsWith(flagPrefix)) {
            setName(text + flagSuffix);
        } else {
            setName(flagPrefix + text + flagSuffix);
        }
    }
}

void RenameDialog::showEvent(QShowEvent* event)
{
    if (type == Type::Class) {
        ui->realnameLabel->hide();
        ui->realnameLineEdit->hide();
        ui->autoRenameCheckBox->setChecked(false);
        ui->autoRenameCheckBox->hide();
        ui->nameLineEdit->setFocus();
        ui->nameLineEdit->selectAll();
        return;
    }

    // Disable auto-rename if realname is not a sub-string of the flagName
    QString flagName = getName();
    QString realname = getRealName();

    if (realname.isEmpty()) {
        setRealName(flagName);
        realname = flagName;
    }

    bool sameNames = flagName == realname;
    bool realnameIsSubstring = flagName.contains(realname);

    // We want realname to be a substring of name. but not an exact match.
    // Otherwise, we'll try to handle the prefixes by ourselves
    if (realnameIsSubstring && !sameNames) {

        // Realname is a substring, we can enable auto-name
        // and do auto-magic when renaming
        ui->autoRenameCheckBox->setChecked(true);

        // Grab the index of realname inside the flag name, usually
        // it will be right after the prefix such as "imp.", "fcn.", "sym.", etc.
        int indexOfSubstring = flagName.indexOf(realname);

        // Grab the prefix, including the separating 'dot'. We'll use this prefix
        // later to construct the new flag name.
        // For the name "sym.helloworld" and realname "helloworld" the prefix
        // will be "sym.".
        flagPrefix = flagName.left(indexOfSubstring);

        if (indexOfSubstring + realname.length() < flagName.length()) {

            // When possible, we would also like to grab the suffix of the flag.
            // For the name "sym.helloworld_2a" and realname "helloworld" the suffix
            // Will be "_2a". Then. when the realname would change to something like
            // "goodbye", the flag name would be "sym.goodbye_2a".
            flagSuffix = flagName.right(indexOfSubstring + realname.length());
        }
    } else {
        int indexOfDot = realname.indexOf('.');
        if (indexOfDot == -1) {
            // loc. is our generic prefix to mark a location.
            // If more info provided to us we can use fcn.,
            // str., etc.
            switch (type) {
            case Type::Function:
                flagPrefix = "fcn.";
                break;
            case Type::String:
                flagPrefix = "str.";
                break;
             default:
                flagPrefix = "loc.";
            }
        } else {
            flagPrefix = realname.left(indexOfDot+1);
        }
        // Enable or disable the auto-rename checkbox based on our
        // confidence with having the prefix.
        ui->autoRenameCheckBox->setChecked(indexOfDot != -1 || sameNames);
    }

    // Focus on realname and select the text to make it quick for the user to rename
    ui->realnameLineEdit->setFocus();
    ui->realnameLineEdit->selectAll();
}

void RenameDialog::accept()
{
    if (!getName().contains('.')) {
        qDebug() << "Potential problematic UID Name for the flag. Flags are better to contain a dot ('.')";
    }
    QDialog::accept();
}
