
#include <QFileDialog>

#include <cutter.h>
#include "SaveProjectDialog.h"
#include "ui_SaveProjectDialog.h"

SaveProjectDialog::SaveProjectDialog(bool quit, QWidget *parent) :
        QDialog(parent),
        ui(new Ui::SaveProjectDialog)
{
    ui->setupUi(this);

    CutterCore *core = CutterCore::getInstance();

    if (quit)
    {
        ui->buttonBox->setStandardButtons(QDialogButtonBox::Save
                                          | QDialogButtonBox::Discard
                                          | QDialogButtonBox::Cancel);
    }
    else
    {
        ui->buttonBox->setStandardButtons(QDialogButtonBox::Save
                                          | QDialogButtonBox::Cancel);
    }

    ui->nameEdit->setText(core->getConfig("prj.name"));
    ui->projectsDirEdit->setText(core->getConfig("dir.projects"));
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
    if(currentDir.startsWith("~"))
    {
        currentDir = QDir::homePath() + currentDir.mid(1);
    }
    dialog.setDirectory(currentDir);

    dialog.setWindowTitle(tr("Select project path (dir.projects)"));

    if(!dialog.exec())
    {
        return;
    }

    QString dir = dialog.selectedFiles().first();
    if (!dir.isEmpty())
    {
        ui->projectsDirEdit->setText(dir);
    }
}

void SaveProjectDialog::on_buttonBox_clicked(QAbstractButton *button)
{
    switch(ui->buttonBox->buttonRole(button))
    {
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
    CutterCore *core = CutterCore::getInstance();
    core->setConfig("dir.projects", ui->projectsDirEdit->text().toUtf8().constData());
    core->setConfig("prj.simple", ui->simpleCheckBox->isChecked());
    core->setConfig("prj.files", ui->filesCheckBox->isChecked());
    core->setConfig("prj.git", ui->gitCheckBox->isChecked());
    core->setConfig("prj.zip", ui->zipCheckBox->isChecked());

    QString projectName = ui->nameEdit->text().trimmed();
    if(!CutterCore::isProjectNameValid(projectName))
    {
        QMessageBox::critical(this, tr("Save project"), tr("Invalid project name."));
        return;
    }

    core->saveProject(projectName);

    QDialog::done(Saved);
}

void SaveProjectDialog::reject()
{
    done(Rejected);
}
