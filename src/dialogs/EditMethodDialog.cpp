#include "EditMethodDialog.h"
#include "ui_EditMethodDialog.h"

EditMethodDialog::EditMethodDialog(bool classFixed, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::EditMethodDialog)
{
    ui->setupUi(this);
    setWindowFlags(windowFlags() & (~Qt::WindowContextHelpButtonHint));

    if (classFixed) {
        classLabel = new QLabel(this);
        ui->formLayout->setItem(0, QFormLayout::FieldRole, new QWidgetItem(classLabel));
    } else {
        classComboBox = new QComboBox(this);
        ui->formLayout->setItem(0, QFormLayout::FieldRole, new QWidgetItem(classComboBox));
        for (auto &cls : Core()->getAllAnalClasses(true)) {
            classComboBox->addItem(cls, cls);
        }
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
    // TODO: do more checks here, for example for name clashes
    return true;
}

void EditMethodDialog::setClass(const QString &className)
{
    if (classComboBox) {
        if (className.isEmpty()) {
            classComboBox->setCurrentIndex(0);
            return;
        }

        for (int i=0; i<classComboBox->count(); i++) {
            QString cls = classComboBox->itemData(i).toString();
            if (cls == className) {
                classComboBox->setCurrentIndex(i);
                break;
            }
        }
    } else {
        classLabel->setText(className);
        fixedClass = className;
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
    if (classComboBox) {
        int index = classComboBox->currentIndex();
        if (index < 0) {
            return nullptr;
        }
        return classComboBox->itemData(index).toString();
    } else {
        return fixedClass;
    }
}

AnalMethodDescription EditMethodDialog::getMethod()
{
    AnalMethodDescription ret;
    ret.name = ui->nameEdit->text();
    ret.addr = Core()->num(ui->addressEdit->text());
    if (!ui->virtualCheckBox->isChecked()) {
        ret.vtableOffset = -1;
    } else {
        ret.vtableOffset = Core()->num(ui->vtableOffsetEdit->text());
    }
    return ret;
}

bool EditMethodDialog::showDialog(const QString &title, bool classFixed, QString *className, AnalMethodDescription *desc, QWidget *parent)
{
    EditMethodDialog dialog(classFixed, parent);
    dialog.setWindowTitle(title);
    dialog.setClass(*className);
    dialog.setMethod(*desc);
    int result = dialog.exec();
    *className = dialog.getClass();
    *desc = dialog.getMethod();
    return result == QDialog::DialogCode::Accepted;
}

void EditMethodDialog::newMethod(QString className, const QString &meth, QWidget *parent)
{
    AnalMethodDescription desc;
    desc.name = meth;
    desc.vtableOffset = -1;
    desc.addr = Core()->getOffset();

    if (!showDialog(tr("Create Method"), false, &className, &desc, parent)) {
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
    if (!showDialog(tr("Edit Method"), false, &classNameCopy, &desc, parent)) {
        return;
    }
    if (desc.name != meth) {
        Core()->renameAnalMethod(className, meth, desc.name);
    }
    Core()->setAnalMethod(className, desc);
}