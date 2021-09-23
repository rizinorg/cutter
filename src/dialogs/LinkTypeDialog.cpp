#include "LinkTypeDialog.h"
#include "ui_LinkTypeDialog.h"

LinkTypeDialog::LinkTypeDialog(QWidget *parent) : QDialog(parent), ui(new Ui::LinkTypeDialog)
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
                RzCoreLocked core(Core());
                ut64 addr = rz_num_math(core->num, address.toUtf8().constData());
                rz_analysis_type_unlink(core->analysis, addr);
            } else {
                // Create link
                RzCoreLocked core(Core());
                ut64 addr = rz_num_math(core->num, address.toUtf8().constData());
                rz_core_types_link(core, type.toUtf8().constData(), addr);
            }
            QDialog::done(r);

            // Seek to the specified address
            Core()->seekAndShow(address);

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

    RzCoreLocked core(Core());
    RzType *link = rz_analysis_type_link_at(core->analysis, address);
    if (!link) {
        return QString();
    }
    RzBaseType *base = rz_type_get_base_type(core->analysis->typedb, link);
    if (!base) {
        return QString();
    }

    return QString(base->name);
}

void LinkTypeDialog::on_exprLineEdit_textChanged(const QString &text)
{
    RVA addr = Core()->math(text);
    if (Core()->isAddressMapped(addr)) {
        ui->addressLineEdit->setText(RzAddressString(addr));
        addrValid = true;
    } else {
        ui->addressLineEdit->setText(tr("Invalid Address"));
        addrValid = false;
    }
}
