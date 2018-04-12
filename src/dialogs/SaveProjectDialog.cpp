
#include <QFileDialog>

#include <Cutter.h>
#include "SaveProjectDialog.h"
#include "ui_SaveProjectDialog.h"
#include "utils/TempConfig.h"
#include "utils/Configuration.h"

SaveProjectDialog::SaveProjectDialog(bool quit, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::SaveProjectDialog)
{
    ui->setupUi(this);

    CutterCore *core = Core();

    if (quit) {
        ui->buttonBox->setStandardButtons(QDialogButtonBox::Save
                                          | QDialogButtonBox::Discard
                                          | QDialogButtonBox::Cancel);
    } else {
        ui->buttonBox->setStandardButtons(QDialogButtonBox::Save
                                          | QDialogButtonBox::Cancel);
    }

    ui->nameEdit->setText(core->getConfig("prj.name"));
    ui->projectsDirEdit->setText(Config()->getDirProjects());
    ui->simpleCheckBox->setChecked(core->getConfigb("prj.simple"));
    ui->filesCheckBox->setChecked(core->getConfigb("prj.files"));
    ui->gitCheckBox->setChecked(core->getConfigb("prj.git"));
    ui->zipCheckBox->setChecked(core->getConfigb("prj.zip"));
}

SaveProjectDialog::~SaveProjectDialog()
{
}

void SaveProjectDialog::on_selectProjectsDirButton_clicked()
{
    QFileDialog dialog(this);
    dialog.setFileMode(QFileDialog::DirectoryOnly);

    QString currentDir = ui->projectsDirEdit->text();
    if (currentDir.startsWith("~")) {
        currentDir = QDir::homePath() + currentDir.mid(1);
    }
    dialog.setDirectory(currentDir);

    dialog.setWindowTitle(tr("Select project path (dir.projects)"));

    if (!dialog.exec()) {
        return;
    }

    QString dir = dialog.selectedFiles().first();
    if (!dir.isEmpty()) {
        ui->projectsDirEdit->setText(dir);
    }
}

void SaveProjectDialog::on_buttonBox_clicked(QAbstractButton *button)
{
    switch (ui->buttonBox->buttonRole(button)) {
    case QDialogButtonBox::DestructiveRole:
        QDialog::done(Destructive);
        break;

    case QDialogButtonBox::RejectRole:
        QDialog::done(Rejected);
        break;

    default:
        break;
    }
}

void SaveProjectDialog::accept()
{
    TempConfig tempConfig;
    Config()->setDirProjects(ui->projectsDirEdit->text().toUtf8().constData());
    tempConfig.set("dir.projects", Config()->getDirProjects())
    .set("prj.simple", ui->simpleCheckBox->isChecked())
    .set("prj.files", ui->filesCheckBox->isChecked())
    .set("prj.git", ui->gitCheckBox->isChecked())
    .set("prj.zip", ui->zipCheckBox->isChecked());

    QString projectName = ui->nameEdit->text().trimmed();
    if (!CutterCore::isProjectNameValid(projectName)) {
        QMessageBox::critical(this, tr("Save project"), tr("Invalid project name."));
        return;
    }

    Core()->saveProject(projectName);

    QDialog::done(Saved);
}

void SaveProjectDialog::reject()
{
    done(Rejected);
}
