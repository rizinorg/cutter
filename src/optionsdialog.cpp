#include "optionsdialog.h"
#include "ui_optionsdialog.h"
#include "mainwindow.h"
#include "newfiledialog.h"
#include "helpers.h"

// TODO: remove us
#include "widgets/memorywidget.h"
#include "widgets/notepad.h"
#include "settings.h"

#include <QSettings>
#include <QFileInfo>
#include <QFileDialog>

OptionsDialog::OptionsDialog(MainWindow *main):
    QDialog(0), // parent may not be main
    ui(new Ui::OptionsDialog),
    analThread(this),
    main(main),
    defaultAnalLevel(1)
{
    ui->setupUi(this);
    setWindowFlags(windowFlags() & (~Qt::WindowContextHelpButtonHint));
    ui->progressBar->setVisible(0);
    ui->statusLabel->setVisible(0);

    ui->analSlider->setValue(defaultAnalLevel);

    // Fill the plugins combo
    asm_plugins = main->core->getAsmPluginNames();
    for (auto plugin : asm_plugins)
        ui->archComboBox->addItem(plugin, plugin);
    ui->archComboBox->setToolTip(main->core->cmd("e? asm.arch").trimmed());

    // cpu combo box
    ui->cpuComboBox->lineEdit()->setPlaceholderText(tr("Auto"));
    ui->cpuComboBox->setToolTip(main->core->cmd("e? asm.cpu").trimmed());
    updateCPUComboBox();

    // os combo box
    for (const auto &plugin : main->core->cmdList("e asm.os=?"))
        ui->kernelComboBox->addItem(plugin, plugin);
    ui->kernelComboBox->setToolTip(main->core->cmd("e? asm.os").trimmed());

    ui->bitsComboBox->setToolTip(main->core->cmd("e? asm.bits").trimmed());

    // Restore settings
    QSettings settings;
    ui->bytesCheckBox->setChecked(settings.value("bytes").toBool());
    ui->descriptionCheckBox->setChecked(settings.value("describe").toBool());
    ui->stackCheckBox->setChecked(settings.value("stackptr").toBool());
    ui->ucaseCheckBox->setChecked(settings.value("ucase").toBool());
    ui->spacyCheckBox->setChecked(settings.value("bbline").toBool());

    ui->hideFrame->setVisible(false);
    ui->analoptionsFrame->setVisible(false);

    updatePDBLayout();

    connect(ui->pdbCheckBox, SIGNAL(stateChanged(int)), this, SLOT(updatePDBLayout()));

    // Add this so the dialog resizes when widgets are shown/hidden
    //this->layout()->setSizeConstraint(QLayout::SetFixedSize);

    connect(&analThread, SIGNAL(finished()), this, SLOT(anal_finished()));

    ui->programLineEdit->setText(main->getFilename());
    QFileInfo fi(this->main->getFilename());
    this->main->core->tryFile(fi.filePath(), fi.isWritable());
}

OptionsDialog::~OptionsDialog()
{
    delete ui;
}

void OptionsDialog::updateCPUComboBox()
{
    QString currentText = ui->cpuComboBox->lineEdit()->text();
    ui->cpuComboBox->clear();

    QString cmd = "e asm.cpu=?";

    QString arch = getSelectedArch();
    if (!arch.isNull())
        cmd += " @a:" + arch;

    ui->cpuComboBox->addItem("");
    ui->cpuComboBox->addItems(main->core->cmdList(cmd));

    ui->cpuComboBox->lineEdit()->setText(currentText);
}

QString OptionsDialog::getSelectedArch()
{
    QVariant archValue = ui->archComboBox->currentData();
    return archValue.isValid() ? archValue.toString() : nullptr;
}

QString OptionsDialog::getSelectedCPU()
{
    QString cpu = ui->cpuComboBox->currentText();
    if (cpu.isNull() || cpu.isEmpty())
        return nullptr;
    return cpu;
}

int OptionsDialog::getSelectedBits()
{
    QString sel_bits = ui->bitsComboBox->currentText();
    if (sel_bits != "Auto")
    {
        return sel_bits.toInt();
    }

    return 0;
}

QString OptionsDialog::getSelectedOS()
{
    QVariant os = ui->kernelComboBox->currentData();
    return os.isValid() ? os.toString() : nullptr;
}

void OptionsDialog::setupAndStartAnalysis(int level, QList<QString> advanced)
{
    ui->analSlider->setValue(level);

    this->setEnabled(0);
    ui->logo->setEnabled(true);

    // Show Progress Bar
    ui->progressBar->setEnabled(1);
    ui->statusLabel->setEnabled(1);
    ui->progressBar->setVisible(1);
    ui->statusLabel->setVisible(1);

    ui->statusLabel->setText(tr("Starting analysis"));
    //ui->progressBar->setValue(5);

    int va = ui->vaCheckBox->isChecked();
    ut64 loadaddr = 0LL;
    ut64 mapaddr = 0LL;

    // Save options in settings
    Settings settings;
    settings.setAsmBytes(ui->bytesCheckBox->isChecked());
    settings.setOpcodeDescription(ui->descriptionCheckBox->isChecked());
    settings.setStackPointer(ui->stackCheckBox->isChecked());
    settings.setUppercaseDisas(ui->ucaseCheckBox->isChecked());
    settings.setSpacy(ui->spacyCheckBox->isChecked());

    main->initUI();

    // Apply options set above in MainWindow
    main->applySettings();

    //
    // Advanced Options
    //

    main->core->setCPU(getSelectedArch(), getSelectedCPU(), getSelectedBits());

    bool rw = false;
    bool load_bininfo = ui->binCheckBox->isChecked();

    if (load_bininfo)
    {
        if (!va)
        {
            va = 2;
            loadaddr = UT64_MAX;
            r_config_set_i(main->core->core()->config, "bin.laddr", loadaddr);
            mapaddr = 0;
        }
    }
    else
    {
        va = false;
        loadaddr = mapaddr = 0;
    }

    //ui->progressBar->setValue(20);
    ui->statusLabel->setText(tr("Loading binary"));
    // options dialog should show the list of archs inside the given fatbin
    int binidx = 0; // index of subbin

    main->addOutput(tr(" > Loading file: ") + main->getFilename());
    main->core->loadFile(main->getFilename(), loadaddr, mapaddr, rw, va, binidx, load_bininfo);
    //ui->progressBar->setValue(40);
    ui->statusLabel->setText(tr("Analysis in progress"));


    QString os = getSelectedOS();
    if (!os.isNull())
    {
        main->core->cmd("e asm.os=" + os);
    }


    if (ui->pdbCheckBox->isChecked())
    {
        main->core->loadPDB(ui->pdbLineEdit->text());
    }


    // Threads stuff
    // connect signal/slot
    analThread.start(main->core, level, advanced);
}

void OptionsDialog::on_closeButton_clicked()
{
    close();
}

void OptionsDialog::on_okButton_clicked()
{
    QList<QString> advanced = QList<QString>();
    if (ui->analSlider->value() == 3)
    {
        if (ui->aa_symbols->isChecked())
        {
            advanced << "aa";
        }
        if (ui->aar_references->isChecked())
        {
            advanced << "aar";
        }
        if (ui->aac_calls->isChecked())
        {
            advanced << "aac";
        }
        if (ui->aan_rename->isChecked())
        {
            advanced << "aan";
        }
        if (ui->aae_emulate->isChecked())
        {
            advanced << "aae";
        }
        if (ui->aat_consecutive->isChecked())
        {
            advanced << "aat";
        }
        if (ui->afta_typeargument->isChecked())
        {
            advanced << "afta";
        }
        if (ui->aaT_aftertrap->isChecked())
        {
            advanced << "aaT";
        }
        if (ui->aap_preludes->isChecked())
        {
            advanced << "aap";
        }
        if (ui->jmptbl->isChecked())
        {
            advanced << "e! anal.jmptbl";
        }
        if (ui->pushret->isChecked())
        {
            advanced << "e! anal.pushret";
        }
    }

    setupAndStartAnalysis(ui->analSlider->value(), advanced);
}

void OptionsDialog::anal_finished()
{
    ui->statusLabel->setText(tr("Loading interface"));
    main->addOutput(tr(" > Analysis finished"));

    QString initial_seek = ui->entry_initialSeek->text();
    if (initial_seek.length() > 0)
    {
        main->core->seek(initial_seek);
    }
    else
    {
        main->core->seek("entry0");
    }

    main->finalizeOpen();
    close();
}

void OptionsDialog::on_cancelButton_clicked()
{
    //delete this->core;
    //this->core = NULL;
    // Close dialog and open OptionsDialog
    delete main;
    close();
    NewFileDialog *n = new NewFileDialog(nullptr);
    n->show();
}

QString OptionsDialog::analysisDescription(int level)
{
    //TODO: replace this with meaningful descriptions
    switch (level)
    {
    case 0:
        return tr("No analysis");
    case 1:
        return tr("Auto-Analysis (aaa)");
    case 2:
        return tr("Auto-Analysis Experimental (aaaa)");
    case 3:
        return tr("Advanced");
    default:
        return tr("Unknown");
    }
}

void OptionsDialog::on_analSlider_valueChanged(int value)
{
    ui->analDescription->setText(tr("Level") + QString(": %1").arg(analysisDescription(value)));
    if (value == 0)
    {
        ui->analCheckBox->setChecked(false);
        ui->analCheckBox->setText(tr("Analysis: Disabled"));
    }
    else
    {
        ui->analCheckBox->setChecked(true);
        ui->analCheckBox->setText(tr("Analysis: Enabled"));
        if (value == 3)
        {
            ui->analoptionsFrame->setVisible(true);
        }
        else
        {
            ui->analoptionsFrame->setVisible(false);
        }
    }
}

void OptionsDialog::on_AdvOptButton_clicked()
{
    if (ui->AdvOptButton->isChecked())
    {
        ui->hideFrame->setVisible(true);
        ui->AdvOptButton->setArrowType(Qt::DownArrow);
    }
    else
    {
        ui->hideFrame->setVisible(false);
        ui->AdvOptButton->setArrowType(Qt::RightArrow);
    }
}

void OptionsDialog::on_analCheckBox_clicked(bool checked)
{
    if (!checked)
        defaultAnalLevel = ui->analSlider->value();
    ui->analSlider->setValue(checked ? defaultAnalLevel : 0);
}

void OptionsDialog::on_archComboBox_currentIndexChanged(int)
{
    updateCPUComboBox();
}

void OptionsDialog::updatePDBLayout()
{
    ui->pdbWidget->setEnabled(ui->pdbCheckBox->isChecked());
}

void OptionsDialog::on_pdbSelectButton_clicked()
{
    QFileDialog dialog(this);
    dialog.setWindowTitle(tr("Select PDB file"));
    dialog.setNameFilters({ tr("PDB file (*.pdb)"), tr("All files (*)") });

    if (!dialog.exec())
    {
        return;
    }

    QString fileName = dialog.selectedFiles().first();

    if (!fileName.isEmpty())
    {
        ui->pdbLineEdit->setText(fileName);
    }
}
