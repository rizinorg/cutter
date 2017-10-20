#include <cutter.h>
#include "SaveProjectDialog.h"
#include "ui_SaveProjectDialog.h"

SaveProjectDialog::SaveProjectDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::SaveProjectDialog)
{
    ui->setupUi(this);

	CutterCore *core = CutterCore::getInstance();

	ui->projectsDirEdit->setText(core->getConfig("dir.projects"));
	ui->filesCheckBox->setChecked(core->getConfigb("prj.files"));
	ui->gitCheckBox->setChecked(core->getConfigb("prj.git"));
	ui->zipCheckBox->setChecked(core->getConfigb("prj.zip"));
}

SaveProjectDialog::~SaveProjectDialog()
{
}

void SaveProjectDialog::on_buttonBox_accepted()
{
	CutterCore *core = CutterCore::getInstance();

	core->setConfig("dir.projects", ui->projectsDirEdit->text().toUtf8().constData());
	core->setConfig("prj.files", ui->filesCheckBox->isChecked());
	core->setConfig("prj.git", ui->gitCheckBox->isChecked());
	core->setConfig("prj.zip", ui->zipCheckBox->isChecked());
}

void SaveProjectDialog::on_buttonBox_rejected()
{
}
