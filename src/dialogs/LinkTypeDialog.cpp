#include "LinkTypeDialog.h"
#include "ui_LinkTypeDialog.h"

LinkTypeDialog::LinkTypeDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::LinkTypeDialog)
{
    addrValid = false;

    ui->setupUi(this);

    setWindowTitle(tr("Link type to address"));

    // Populate the structureTypeComboBox
    ui->structureTypeComboBox->addItem(tr("(No Type)"));
    for (const TypeDescription &thisType : Core()->getAllStructs()) {
        ui->structureTypeComboBox->addItem(thisType.type);
    }
}

LinkTypeDialog::~LinkTypeDialog()
{
    delete ui;
}

void LinkTypeDialog::setDefaultType(const QString &type)
{
    int index = ui->structureTypeComboBox->findText(type);
    if (index != -1) {
        // index is valid so set is as the default
        ui->structureTypeComboBox->setCurrentIndex(index);
    }
}

bool LinkTypeDialog::setDefaultAddress(const QString &address)
{
    // setting the text here will trigger on_exprLineEdit_textChanged, which will update addrValid
    ui->exprLineEdit->setText(address);

    if (!addrValid) {
        return false;
    }

    // check if the current address is already linked to a type and set it as default
    QString type = findLinkedType(Core()->math(ui->addressLineEdit->text()));
    if (!type.isEmpty()) {
        setDefaultType(type);
    }
    return true;
}


void LinkTypeDialog::done(int r)
{
    if (r == QDialog::Accepted) {
        QString address = ui->addressLineEdit->text();
        if (Core()->isAddressMapped(Core()->math(address))) {
            // Address is valid so link the type to the address
            QString type = ui->structureTypeComboBox->currentText();
            if (type == tr("(No Type)")) {
                // Delete link
                Core()->cmdRaw("tl- " + address);
            } else {
                // Create link
                Core()->cmdRaw(QString("tl %1 = %2").arg(type).arg(address));
            }
            QDialog::done(r);

            // Seek to the specified address
            Core()->seek(address);

            // Refresh the views
            emit Core()->refreshCodeViews();
            return;
        }

        // Address is invalid so display error message
        QMessageBox::warning(this, tr("Error"), tr("The given address is invalid"));
    } else {
        QDialog::done(r);
    }
}

QString LinkTypeDialog::findLinkedType(RVA address)
{
    if (address == RVA_INVALID) {
        return QString();
    }

    QString ret = Core()->cmdRaw(QString("tls %1").arg(address));
    if (ret.isEmpty()) {
        // return empty string since the current address is not linked to a type
        return QString();
    }

    // Extract the given type from returned data
    // TODO: Implement "tlsj" in radare2 or some other function to directly get linked type
    QString s = ret.section(QLatin1Char('\n'), 0, 0);
    return s.mid(1, s.size() - 2);
}

void LinkTypeDialog::on_exprLineEdit_textChanged(const QString &text)
{
    RVA addr = Core()->math(text);
    if (Core()->isAddressMapped(addr)) {
        ui->addressLineEdit->setText(RAddressString(addr));
        addrValid = true;
    } else {
        ui->addressLineEdit->setText(tr("Invalid Address"));
        addrValid = false;
    }
}
