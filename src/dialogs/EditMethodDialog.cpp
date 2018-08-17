#include "EditMethodDialog.h"
#include "ui_EditMethodDialog.h"

EditMethodDialog::EditMethodDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::EditMethodDialog)
{
    ui->setupUi(this);
    setWindowFlags(windowFlags() & (~Qt::WindowContextHelpButtonHint));

    ui->classComboBox->clear();
    /* TODO for (auto &cls : Core()->getAllClassesFromAnal()) {
        ui->classComboBox->addItem(cls.name, QVariant::fromValue<BinClassDescription>(cls));
    }*/

    updateVirtualUI();
    validateInput();

    connect(ui->virtualCheckBox, &QCheckBox::stateChanged, this, &EditMethodDialog::updateVirtualUI);
    connect(ui->nameEdit, &QLineEdit::textChanged, this, &EditMethodDialog::validateInput);
}

EditMethodDialog::~EditMethodDialog() {}

void EditMethodDialog::on_buttonBox_accepted()
{
}

void EditMethodDialog::on_buttonBox_rejected()
{
    close();
}

void EditMethodDialog::updateVirtualUI()
{
    bool enabled = ui->virtualCheckBox->isChecked();
    ui->vtableOffsetEdit->setEnabled(enabled);
    ui->vtableOffsetLabel->setEnabled(enabled);
}

void EditMethodDialog::validateInput()
{
    for (auto button : ui->buttonBox->buttons()) {
        if (ui->buttonBox->buttonRole(button) == QDialogButtonBox::AcceptRole) {
            button->setEnabled(inputValid());
            return;
        }
    }
}

bool EditMethodDialog::inputValid()
{
    if (ui->nameEdit->text().isEmpty()) {
        return false;
    }
    return true;
}

void EditMethodDialog::setClass(const QString &className)
{
    if (className.isEmpty()) {
        ui->classComboBox->setCurrentIndex(0);
        return;
    }

    for (int i=0; i<ui->classComboBox->count(); i++) {
        BinClassDescription cls = ui->classComboBox->itemData(i).value<BinClassDescription>();
        if (cls.name == className) {
            ui->classComboBox->setCurrentIndex(i);
            break;
        }
    }

    validateInput();
}

void EditMethodDialog::setMethod(const ClassMethodDescription &meth)
{
    ui->nameEdit->setText(meth.name);
    ui->addressEdit->setText(meth.addr != RVA_INVALID ? RAddressString(meth.addr) : nullptr);

    if (meth.vtableOffset >= 0) {
        ui->virtualCheckBox->setChecked(true);
        ui->vtableOffsetEdit->setText(QString::number(meth.vtableOffset));
    } else {
        ui->virtualCheckBox->setChecked(false);
        ui->vtableOffsetEdit->setText(nullptr);
    }

    updateVirtualUI();
    validateInput();
}

QString EditMethodDialog::getClass()
{
    int index = ui->classComboBox->currentIndex();
    if (index < 0) {
        return nullptr;
    }
    return ui->classComboBox->itemData(index).value<BinClassDescription>().name;
}

ClassMethodDescription EditMethodDialog::getMethod()
{
    ClassMethodDescription ret;
    ret.name = ui->nameEdit->text();
    ret.addr = Core()->num(ui->addressEdit->text());
    if (ui->virtualCheckBox->isChecked()) {
        ret.vtableOffset = -1;
    } else {
        ret.vtableOffset = Core()->num(ui->vtableOffsetEdit->text());
    }
    return ret;
}

bool EditMethodDialog::showDialog(const QString &title, const QString &className, ClassMethodDescription *meth, QWidget *parent)
{
    auto dialog = new EditMethodDialog(parent);
    dialog->setWindowTitle(title);
    dialog->setClass(className);
    dialog->setMethod(*meth);
    int result = dialog->exec();
    *meth = dialog->getMethod();
    return result == QDialog::DialogCode::Accepted;
}

void EditMethodDialog::newMethod(const QString &className, ClassMethodDescription meth)
{
    if (!showDialog(tr("Create Method"), className, &meth)) {
        return;
    }
    Core()->setClassMethod(className, meth);
}

void EditMethodDialog::editMethod(const QString &className, ClassMethodDescription meth)
{
    QString oldName = meth.name;
    if (!showDialog(tr("Edit Method"), className, &meth)) {
        return;
    }
    if (meth.name != oldName) {
        Core()->renameClassMethod(className, oldName, meth.name);
    }
    Core()->setClassMethod(className, meth);
}