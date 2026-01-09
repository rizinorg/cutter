#include "NativeDebugDialog.h"
#include "ui_NativeDebugDialog.h"
#include "shortcuts/ShortcutManager.h"
#include "ProfileDirectivesDialog.h"

#include <QMessageBox>
#include <QShortcut>
#include <QFileDialog>
#include <QTextStream>

NativeDebugDialog::NativeDebugDialog(QWidget *parent)
    : QDialog(parent), ui(new Ui::NativeDebugDialog)
{
    ui->setupUi(this);
    setWindowFlags(windowFlags() & (~Qt::WindowContextHelpButtonHint));
    auto shortcut = Shortcuts()->makeQShortcut("Debug.accept", ui->argEdit);
    shortcut->setContext(Qt::ShortcutContext::WidgetShortcut);
    connect(shortcut, &QShortcut::activated, this, &QDialog::accept);

    connect(ui->profileCheckbox, &QCheckBox::toggled, this,
            &NativeDebugDialog::profileCheckboxToggled);
    connect(ui->profilePathBtn, &QPushButton::clicked, this,
            &NativeDebugDialog::profilePathBtnClicked);
    connect(ui->directiveListBtn, &QPushButton::clicked, this,
            &NativeDebugDialog::directiveListBtnClicked);

    profileCheckboxToggled(ui->profileCheckbox->isChecked());
}

NativeDebugDialog::~NativeDebugDialog() {}

QString NativeDebugDialog::getArgs() const
{
    return ui->argEdit->toPlainText();
}

void NativeDebugDialog::setArgs(const QString &args)
{
    ui->argEdit->setPlainText(args);
    ui->argEdit->selectAll();
}

void NativeDebugDialog::setProfilePath(const QString &profilePath)
{
    ui->profilePathEdit->setText(profilePath);
    setFileContents(profilePath);
}

void NativeDebugDialog::profileCheckboxToggled(bool checked)
{
    if (checked && showWarningWhenChecked) {
        showWarning(profilePath);
    }

    ui->argEdit->setEnabled(!checked);
    ui->argText->setEnabled(!checked);

    ui->profileText->setEnabled(checked);
    ui->profilePathEdit->setEnabled(checked);
    ui->profilePathBtn->setEnabled(checked);
    ui->directiveText->setEnabled(checked);
    ui->directiveEdit->setEnabled(checked);
}

QString NativeDebugDialog::getProfilePath() const
{
    return ui->profilePathEdit->text();
}

QString NativeDebugDialog::getDirectives() const
{
    return ui->directiveEdit->toPlainText();
}

DebugConfigMethod NativeDebugDialog::getSelectedMethod() const
{
    return !ui->profileCheckbox->isChecked()
            ? DebugConfigMethod::CommandLine
            : (ui->directiveEdit->document()->isModified() ? DebugConfigMethod::RzRunDirectives
                                                           : DebugConfigMethod::RzRunProfile);
}

void NativeDebugDialog::profilePathBtnClicked()
{
    QString filePath =
            QFileDialog::getOpenFileName(this, tr("Open RzRun Profile"), QString(),
                                         tr("RzRun Profiles (*.rz *.rrz);;All Files (*)"));
    setFileContents(filePath);
}

void NativeDebugDialog::setFileContents(const QString &filePath)
{
    if (filePath.isEmpty()) {
        return;
    }

    ui->profilePathEdit->setText(filePath);

    QFile file(filePath);
    if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QTextStream in(&file);
        QString directives = in.readAll();

        // These directives are loaded from the profile
        // If unmodified: 'dbg.profile' will point directly to the original file path
        // If modified: A temporary file containing the new directives will be created and used as
        // the 'dbg.profile' value instead
        ui->directiveEdit->setPlainText(directives);
        ui->directiveEdit->document()->setModified(false);

        file.close();
    } else if (ui->profileCheckbox->isChecked()) {
        showWarning(filePath);
    } else {
        showWarningWhenChecked = true;
        profilePath = filePath;
    }
}

void NativeDebugDialog::directiveListBtnClicked()
{
    ProfileDirectivesDialog pdd(this);
    pdd.exec();
}

void NativeDebugDialog::showWarning(const QString &filePath)
{
    QMessageBox::warning(this, tr("Error"), tr("Could not open file: %1").arg(filePath));
}
