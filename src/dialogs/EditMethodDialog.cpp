#include "EditMethodDialog.h"
#include "ui_EditMethodDialog.h"

EditMethodDialog::EditMethodDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::EditMethodDialog)
{
    ui->setupUi(this);
    setWindowFlags(windowFlags() & (~Qt::WindowContextHelpButtonHint));

    ui->classComboBox->clear();
    for (auto &cls : Core()->getAllAnalClasses()) {
        ui->classComboBox->addItem(cls, cls);
    }

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
        QString cls = ui->classComboBox->itemData(i).toString();
        if (cls == className) {
            ui->classComboBox->setCurrentIndex(i);
            break;
        }
    }

    validateInput();
}

void EditMethodDialog::setMethod(const AnalMethodDescription &desc)
{
    ui->nameEdit->setText(desc.name);
    ui->addressEdit->setText(desc.addr != RVA_INVALID ? RAddressString(desc.addr) : nullptr);

    if (desc.vtableOffset >= 0) {
        ui->virtualCheckBox->setChecked(true);
        ui->vtableOffsetEdit->setText(QString::number(desc.vtableOffset));
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

AnalMethodDescription EditMethodDialog::getMethod()
{
    AnalMethodDescription ret;
    ret.name = ui->nameEdit->text();
    ret.addr = Core()->num(ui->addressEdit->text());
    if (ui->virtualCheckBox->isChecked()) {
        ret.vtableOffset = -1;
    } else {
        ret.vtableOffset = Core()->num(ui->vtableOffsetEdit->text());
    }
    return ret;
}

bool EditMethodDialog::showDialog(const QString &title, QString *className, AnalMethodDescription *desc, QWidget *parent)
{
    auto dialog = new EditMethodDialog(parent);
    dialog->setWindowTitle(title);
    dialog->setClass(*className);
    dialog->setMethod(*desc);
    int result = dialog->exec();
    *className = dialog->getClass();
    *desc = dialog->getMethod();
    return result == QDialog::DialogCode::Accepted;
}

void EditMethodDialog::newMethod(QString className, const QString &meth, QWidget *parent)
{
    AnalMethodDescription desc;
    desc.name = meth;
    desc.vtableOffset = -1;
    desc.addr = Core()->getOffset();

    if (!showDialog(tr("Create Method"), &className, &desc, parent)) {
        return;
    }

    Core()->setAnalMethod(className, desc);
}

void EditMethodDialog::editMethod(const QString &className, const QString &meth, QWidget *parent)
{
    AnalMethodDescription desc;
    if (!Core()->getAnalMethod(className, meth, &desc)) {
        return;
    }

    QString classNameCopy = className;
    if (!showDialog(tr("Edit Method"), &classNameCopy, &desc, parent)) {
        return;
    }
    if (desc.name != meth) {
        Core()->renameAnalMethod(className, meth, desc.name);
    }
    Core()->setAnalMethod(className, desc);
}