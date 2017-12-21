#include "OptionsDialog.h"
#include "ui_OptionsDialog.h"
#include "MainWindow.h"
#include "dialogs/NewFileDialog.h"
#include "utils/Helpers.h"

// TODO: remove us
#include "widgets/Notepad.h"

#include <QSettings>
#include <QFileInfo>
#include <QFileDialog>

// TODO Get rid of MainWindow
OptionsDialog::OptionsDialog(MainWindow *main):
    QDialog(0), // parent may not be main
    analThread(this),
    main(main),
    core(CutterCore::getInstance()),
    defaultAnalLevel(1),
    ui(new Ui::OptionsDialog)
{
    ui->setupUi(this);
    setWindowFlags(windowFlags() & (~Qt::WindowContextHelpButtonHint));
    ui->progressBar->setVisible(0);
    ui->statusLabel->setVisible(0);
    ui->elapsedLabel->setVisible(0);

    QString logoFile = (palette().window().color().value() < 127) ? ":/img/cutter_white_plain.svg" : ":/img/cutter_plain.svg";
    ui->logoSvgWidget->load(logoFile);

    ui->analSlider->setValue(defaultAnalLevel);

    // Fill the plugins combo
    asm_plugins = core->getAsmPluginNames();
    for (auto plugin : asm_plugins)
        ui->archComboBox->addItem(plugin, plugin);
    ui->archComboBox->setToolTip(core->cmd("e? asm.arch").trimmed());

    // cpu combo box
    ui->cpuComboBox->lineEdit()->setPlaceholderText(tr("Auto"));
    ui->cpuComboBox->setToolTip(core->cmd("e? asm.cpu").trimmed());
    updateCPUComboBox();

    // os combo box
    for (const auto &plugin : core->cmdList("e asm.os=?"))
        ui->kernelComboBox->addItem(plugin, plugin);
    ui->kernelComboBox->setToolTip(core->cmd("e? asm.os").trimmed());

    ui->bitsComboBox->setToolTip(core->cmd("e? asm.bits").trimmed());


    for (auto plugin : core->getRBinPluginDescriptions("bin"))
        ui->formatComboBox->addItem(plugin.name, QVariant::fromValue(plugin));

    ui->hideFrame->setVisible(false);
    ui->analoptionsFrame->setVisible(false);

    updatePDBLayout();

    connect(ui->pdbCheckBox, SIGNAL(stateChanged(int)), this, SLOT(updatePDBLayout()));

    // Add this so the dialog resizes when widgets are shown/hidden
    //this->layout()->setSizeConstraint(QLayout::SetFixedSize);

    connect(&analThread, SIGNAL(finished()), this, SLOT(anal_finished()));
    connect(ui->cancelButton, SIGNAL(clicked()), this, SLOT(reject()));

    ui->programLineEdit->setText(main->getFilename());
    QFileInfo fi(this->main->getFilename());
    this->core->tryFile(fi.filePath(), fi.isWritable());
}

OptionsDialog::~OptionsDialog() {}

void OptionsDialog::updateCPUComboBox()
{
    QString currentText = ui->cpuComboBox->lineEdit()->text();
    ui->cpuComboBox->clear();

    QString cmd = "e asm.cpu=?";

    QString arch = getSelectedArch();
    if (!arch.isNull())
        cmd += " @a:" + arch;

    ui->cpuComboBox->addItem("");
    ui->cpuComboBox->addItems(core->cmdList(cmd));

    ui->cpuComboBox->lineEdit()->setText(currentText);
}

void OptionsDialog::setInteractionEnabled(bool enabled)
{
    ui->optionsWidget->setEnabled(enabled);
    ui->okButton->setEnabled(enabled);
    ui->cancelButton->setEnabled(enabled);
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

    setInteractionEnabled(false);

    // Show Progress Bar
    ui->progressBar->setVisible(true);
    ui->statusLabel->setVisible(true);
    ui->elapsedLabel->setVisible(true);

    ui->statusLabel->setText(tr("Starting analysis"));
    //ui->progressBar->setValue(5);

    main->initUI();

    // Timer for showing elapsed analysis time.
    analTimer.setInterval(1000);
    analTimer.setSingleShot(false);
    analTimer.start();
    analElapsedTimer.start();

    updateProgressTimer();
    connect(&analTimer, SIGNAL(timeout()), this, SLOT(updateProgressTimer()));

    // Threads stuff
    // connect signal/slot
    connect(&analThread, &AnalThread::updateProgress, this, &OptionsDialog::updateProgress);
    analThread.start(main, level, advanced);
}

void OptionsDialog::updateProgressTimer()
{
    int secondsElapsed = (analElapsedTimer.elapsed()+500)/1000;
    int minutesElapsed = secondsElapsed / 60;
    int hoursElapsed = minutesElapsed / 60;

    QString label = tr("Running for") + " ";
    if(hoursElapsed)
    {
        label += tr("%n hour", "%n hours", hoursElapsed);
        label += " ";
    }
    if(minutesElapsed)
    {
        label += tr("%n minute", "%n minutes", minutesElapsed % 60);
        label += " ";
    }
    label += tr("%n seconds", "%n second", secondsElapsed % 60);
    ui->elapsedLabel->setText(label);
}

void OptionsDialog::updateProgress(const QString &status)
{
    ui->statusLabel->setText(status);
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
        if (ui->aab_basicblocks->isChecked())
        {
            advanced << "aab";
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
        if (ui->hasnext->isChecked())
        {
            advanced << "e! anal.hasnext";
        }
    }

    setupAndStartAnalysis(ui->analSlider->value(), advanced);
}

void OptionsDialog::anal_finished()
{
    ui->statusLabel->setText(tr("Loading interface"));
    main->addOutput(tr(" > Analysis finished"));

    main->finalizeOpen();
    done(0);
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

void OptionsDialog::reject()
{
    delete main;
    done(0);
    NewFileDialog *n = new NewFileDialog(nullptr);
    n->show();
}
