#include "EditMethodDialog.h"
#include "ui_EditMethodDialog.h"

#include <QComboBox>

EditMethodDialog::EditMethodDialog(bool classFixed, QWidget *parent)
    : QDialog(parent), ui(new Ui::EditMethodDialog)
{
    ui->setupUi(this);
    setWindowFlags(windowFlags() & (~Qt::WindowContextHelpButtonHint));

    if (classFixed) {
        classLabel = new QLabel(this);
        ui->formLayout->setItem(0, QFormLayout::FieldRole, new QWidgetItem(classLabel));
    } else {
        classComboBox = new QComboBox(this);
        ui->formLayout->setItem(0, QFormLayout::FieldRole, new QWidgetItem(classComboBox));
        for (auto &cls : Core()->getAllAnalysisClasses(true)) {
            classComboBox->addItem(cls, cls);
        }
    }

    updateVirtualUI();
    validateInput();

    connect(ui->virtualCheckBox, &QCheckBox::stateChanged, this,
            &EditMethodDialog::updateVirtualUI);
    connect(ui->nameEdit, &QLineEdit::textChanged, this, &EditMethodDialog::validateInput);
    connect(ui->realNameEdit, &QLineEdit::textChanged, this, &EditMethodDialog::updateName);
    connect(ui->autoRenameCheckBox, &QCheckBox::stateChanged, this,
            &EditMethodDialog::updateAutoRenameEnabled);
}

EditMethodDialog::~EditMethodDialog() {}

void EditMethodDialog::on_buttonBox_accepted() {}

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

void EditMethodDialog::updateName()
{
    if (ui->autoRenameCheckBox->isChecked()) {
        ui->nameEdit->setText(convertRealNameToName(ui->realNameEdit->text()));
    }

    validateInput();
}

void EditMethodDialog::updateAutoRenameEnabled()
{
    ui->nameEdit->setEnabled(!ui->autoRenameCheckBox->isChecked());

    if (ui->autoRenameCheckBox->isChecked()) {
        ui->nameEdit->setText(convertRealNameToName(ui->realNameEdit->text()));
    }
}

bool EditMethodDialog::inputValid()
{
    if (ui->nameEdit->text().isEmpty() || ui->realNameEdit->text().isEmpty()) {
        return false;
    }
    // TODO: do more checks here, for example for name clashes
    return true;
}

QString EditMethodDialog::convertRealNameToName(const QString &realName)
{
    return fromOwnedCharPtr(rz_str_sanitize_sdb_key(realName.toUtf8().constData()));
}

void EditMethodDialog::setClass(const QString &className)
{
    if (classComboBox) {
        if (className.isEmpty()) {
            classComboBox->setCurrentIndex(0);
            return;
        }

        for (int i = 0; i < classComboBox->count(); i++) {
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

void EditMethodDialog::setMethod(const AnalysisMethodDescription &desc)
{
    ui->nameEdit->setText(desc.name);
    ui->realNameEdit->setText(desc.realName);
    ui->addressEdit->setText(desc.addr != RVA_INVALID ? RzAddressString(desc.addr) : nullptr);

    if (desc.vtableOffset >= 0) {
        ui->virtualCheckBox->setChecked(true);
        ui->vtableOffsetEdit->setText(QString::number(desc.vtableOffset));
    } else {
        ui->virtualCheckBox->setChecked(false);
        ui->vtableOffsetEdit->setText(nullptr);
    }

    // Check if auto-rename should be enabled
    bool enableAutoRename = ui->nameEdit->text().isEmpty()
            || ui->nameEdit->text() == convertRealNameToName(ui->realNameEdit->text());
    ui->autoRenameCheckBox->setChecked(enableAutoRename);

    // Set focus to real name edit widget if auto-rename is enabled
    if (enableAutoRename) {
        ui->realNameEdit->setFocus();
    }

    updateVirtualUI();
    validateInput();
}

QString EditMethodDialog::getClass() const
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

AnalysisMethodDescription EditMethodDialog::getMethod() const
{
    AnalysisMethodDescription ret;
    ret.name = ui->nameEdit->text();
    ret.realName = ui->realNameEdit->text();
    ret.addr = Core()->num(ui->addressEdit->text());
    if (!ui->virtualCheckBox->isChecked()) {
        ret.vtableOffset = -1;
    } else {
        ret.vtableOffset = Core()->num(ui->vtableOffsetEdit->text());
    }
    return ret;
}

bool EditMethodDialog::showDialog(const QString &title, bool classFixed, QString *className,
                                  AnalysisMethodDescription *desc, QWidget *parent)
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
    AnalysisMethodDescription desc;
    desc.name = convertRealNameToName(meth);
    desc.realName = meth;
    desc.vtableOffset = -1;
    desc.addr = Core()->getOffset();

    if (!showDialog(tr("Create Method"), false, &className, &desc, parent)) {
        return;
    }

    Core()->setAnalysisMethod(className, desc);
}

void EditMethodDialog::editMethod(const QString &className, const QString &meth, QWidget *parent)
{
    AnalysisMethodDescription desc;
    if (!Core()->getAnalysisMethod(className, meth, &desc)) {
        return;
    }

    QString classNameCopy = className;
    if (!showDialog(tr("Edit Method"), false, &classNameCopy, &desc, parent)) {
        return;
    }
    if (desc.name != meth) {
        Core()->renameAnalysisMethod(className, meth, desc.name);
    }
    Core()->setAnalysisMethod(className, desc);
}
