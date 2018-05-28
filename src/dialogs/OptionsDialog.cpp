
#include "utils/AsyncTask.h"

#include "OptionsDialog.h"
#include "ui_OptionsDialog.h"
#include "MainWindow.h"
#include "dialogs/NewFileDialog.h"
#include "dialogs/AsyncTaskDialog.h"
#include "utils/Helpers.h"

#include <QSettings>
#include <QFileInfo>
#include <QFileDialog>

OptionsDialog::OptionsDialog(MainWindow *main):
    QDialog(0), // parent may not be main
    main(main),
    core(Core()),
    defaultAnalLevel(1),
    ui(new Ui::OptionsDialog)
{
    ui->setupUi(this);
    setWindowFlags(windowFlags() & (~Qt::WindowContextHelpButtonHint));
    ui->logoSvgWidget->load(Config()->getLogoFile());
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

    ui->entry_analbb->setToolTip(core->cmd("e? anal.bb.maxsize").trimmed());

    for (auto plugin : core->getRBinPluginDescriptions("bin"))
        ui->formatComboBox->addItem(plugin.name, QVariant::fromValue(plugin));

    ui->hideFrame->setVisible(false);
    ui->analoptionsFrame->setVisible(false);

    updatePDBLayout();

    connect(ui->pdbCheckBox, SIGNAL(stateChanged(int)), this, SLOT(updatePDBLayout()));

    updateScriptLayout();
    
    connect(ui->scriptCheckBox, SIGNAL(stateChanged(int)), this, SLOT(updateScriptLayout()));

    connect(ui->cancelButton, SIGNAL(clicked()), this, SLOT(reject()));

    ui->programLineEdit->setText(main->getFilename());
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
    if (sel_bits != "Auto") {
        return sel_bits.toInt();
    }

    return 0;
}

int OptionsDialog::getSelectedBBSize()
{
    QString sel_bbsize = ui->entry_analbb->text();
    bool ok;
    int bbsize = sel_bbsize.toInt(&ok);
    if (ok)
        return bbsize;
    return 1024;
}

InitialOptions::Endianness OptionsDialog::getSelectedEndianness()
{
    switch (ui->endiannessComboBox->currentIndex()) {
    case 1:
        return InitialOptions::Endianness::Little;
    case 2:
        return InitialOptions::Endianness::Big;
    default:
        return InitialOptions::Endianness::Auto;
    }
}

QString OptionsDialog::getSelectedOS()
{
    QVariant os = ui->kernelComboBox->currentData();
    return os.isValid() ? os.toString() : nullptr;
}

QList<QString> OptionsDialog::getSelectedAdvancedAnalCmds()
{
    QList<QString> advanced = QList<QString>();
    if (ui->analSlider->value() == 3) {
        if (ui->aa_symbols->isChecked()) {
            advanced << "aa";
        }
        if (ui->aar_references->isChecked()) {
            advanced << "aar";
        }
        if (ui->aac_calls->isChecked()) {
            advanced << "aac";
        }
        if (ui->aab_basicblocks->isChecked()) {
            advanced << "aab";
        }
        if (ui->aan_rename->isChecked()) {
            advanced << "aan";
        }
        if (ui->aae_emulate->isChecked()) {
            advanced << "aae";
        }
        if (ui->aat_consecutive->isChecked()) {
            advanced << "aat";
        }
        if (ui->afta_typeargument->isChecked()) {
            advanced << "afta";
        }
        if (ui->aaT_aftertrap->isChecked()) {
            advanced << "aaT";
        }
        if (ui->aap_preludes->isChecked()) {
            advanced << "aap";
        }
        if (ui->jmptbl->isChecked()) {
            advanced << "e! anal.jmptbl";
        }
        if (ui->pushret->isChecked()) {
            advanced << "e! anal.pushret";
        }
        if (ui->hasnext->isChecked()) {
            advanced << "e! anal.hasnext";
        }
    }
    return advanced;
}

void OptionsDialog::setupAndStartAnalysis(int level, QList<QString> advanced)
{
    ui->analSlider->setValue(level);

    main->initUI();

    InitialOptions options;

    options.filename = main->getFilename();

    // Where the bin header is located in the file (-B)
    if (ui->entry_loadOffset->text().length() > 0) {
        options.binLoadAddr = Core()->math(ui->entry_loadOffset->text());
    }

    options.mapAddr = Core()->math(ui->entry_mapOffset->text());      // Where to map the file once loaded (-m)
    options.arch = getSelectedArch();
    options.cpu = getSelectedCPU();
    options.bits = getSelectedBits();
    options.os = getSelectedOS();
    options.writeEnabled = ui->writeCheckBox->isChecked();
    options.loadBinInfo = !ui->binCheckBox->isChecked();
    QVariant forceBinPluginData = ui->formatComboBox->currentData();
    if (!forceBinPluginData.isNull()) {
        RBinPluginDescription pluginDesc = forceBinPluginData.value<RBinPluginDescription>();
        options.forceBinPlugin = pluginDesc.name;
    }
    options.demangle = ui->demangleCheckBox->isChecked();
    if (ui->pdbCheckBox->isChecked()) {
        options.pdbFile = ui->pdbLineEdit->text();
    }
    if (ui->scriptCheckBox->isChecked()) {
        options.script = ui->scriptLineEdit->text();
    }
    options.endian = getSelectedEndianness();
    options.bbsize = getSelectedBBSize();

    switch(level) {
        case 1:
            options.analCmd = { "aaa" };
            break;
        case 2:
            options.analCmd = { "aaaa" };
            break;
        case 3:
            options.analCmd = advanced;
            break;
        default:
            options.analCmd = {};
            break;
    }


    AnalTask *analTask = new AnalTask();
    analTask->setOptions(options);

    connect(analTask, &AnalTask::openFileFailed, main, &MainWindow::openNewFileFailed);
    connect(analTask, &AsyncTask::finished, main, &MainWindow::finalizeOpen);

    AsyncTask::Ptr analTaskPtr(analTask);

    Core()->getAsyncTaskManager()->start(analTaskPtr);

    AsyncTaskDialog *taskDialog = new AsyncTaskDialog(analTaskPtr);
    taskDialog->setAttribute(Qt::WA_DeleteOnClose);
    taskDialog->show();

    done(0);
}

void OptionsDialog::on_okButton_clicked()
{
    setupAndStartAnalysis(ui->analSlider->value(), getSelectedAdvancedAnalCmds());
}

void OptionsDialog::closeEvent(QCloseEvent *event)
{
    event->accept();
}

QString OptionsDialog::analysisDescription(int level)
{
    //TODO: replace this with meaningful descriptions
    switch (level) {
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
    if (value == 0) {
        ui->analCheckBox->setChecked(false);
        ui->analCheckBox->setText(tr("Analysis: Disabled"));
    } else {
        ui->analCheckBox->setChecked(true);
        ui->analCheckBox->setText(tr("Analysis: Enabled"));
        if (value == 3) {
            ui->analoptionsFrame->setVisible(true);
        } else {
            ui->analoptionsFrame->setVisible(false);
        }
    }
}

void OptionsDialog::on_AdvOptButton_clicked()
{
    if (ui->AdvOptButton->isChecked()) {
        ui->hideFrame->setVisible(true);
        ui->AdvOptButton->setArrowType(Qt::DownArrow);
    } else {
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

    if (!dialog.exec()) {
        return;
    }

    QString fileName = dialog.selectedFiles().first();

    if (!fileName.isEmpty()) {
        ui->pdbLineEdit->setText(fileName);
    }
}


void OptionsDialog::updateScriptLayout()
{
    ui->scriptWidget->setEnabled(ui->scriptCheckBox->isChecked());
}

void OptionsDialog::on_scriptSelectButton_clicked()
{
    QFileDialog dialog(this);
    dialog.setWindowTitle(tr("Select radare2 script file"));
    dialog.setNameFilters({ tr("Script file (*.r2)"), tr("All files (*)") });

    if (!dialog.exec()) {
        return;
    }

    QString fileName = dialog.selectedFiles().first();

    if (!fileName.isEmpty()) {
        ui->scriptLineEdit->setText(fileName);
    }
}


void OptionsDialog::reject()
{
    done(0);
    NewFileDialog *n = new NewFileDialog(nullptr);
    n->show();
}
