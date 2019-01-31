
#include "common/AsyncTask.h"

#include "InitialOptionsDialog.h"
#include "ui_InitialOptionsDialog.h"
#include "MainWindow.h"
#include "dialogs/NewFileDialog.h"
#include "dialogs/AsyncTaskDialog.h"
#include "common/Helpers.h"

#include <QSettings>
#include <QFileInfo>
#include <QFileDialog>

InitialOptionsDialog::InitialOptionsDialog(MainWindow *main):
    QDialog(0), // parent must not be main
    ui(new Ui::InitialOptionsDialog),
    main(main),
    core(Core())
{
    ui->setupUi(this);
    setWindowFlags(windowFlags() & (~Qt::WindowContextHelpButtonHint));
    ui->logoSvgWidget->load(Config()->getLogoFile());

    // Fill the plugins combo
    asm_plugins = core->getAsmPluginNames();
    for (const auto &plugin : asm_plugins)
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

    for (const auto &plugin : core->getRBinPluginDescriptions("bin"))
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

InitialOptionsDialog::~InitialOptionsDialog() {}

void InitialOptionsDialog::updateCPUComboBox()
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

void InitialOptionsDialog::loadOptions(const InitialOptions &options)
{
    if (options.analCmd.isEmpty()) {
        analLevel = 0;
    } else if (options.analCmd == QList<QString>({ "aaa" })) {
        analLevel = 1;
    } else if (options.analCmd == QList<QString>({ "aaaa" })) {
        analLevel = 2;
    } else {
        analLevel = 3;
        // TODO: These checks must always be in sync with getSelectedAdvancedAnalCmds(), which is dangerous
        ui->aa_symbols->setChecked(options.analCmd.contains("aa"));
        ui->aar_references->setChecked(options.analCmd.contains("aar"));
        ui->aac_calls->setChecked(options.analCmd.contains("aac"));
        ui->aab_basicblocks->setChecked(options.analCmd.contains("aab"));
        ui->aan_rename->setChecked(options.analCmd.contains("aan"));
        ui->aae_emulate->setChecked(options.analCmd.contains("aae"));
        ui->aat_consecutive->setChecked(options.analCmd.contains("aat"));
        ui->afta_typeargument->setChecked(options.analCmd.contains("afta"));
        ui->aaT_aftertrap->setChecked(options.analCmd.contains("aaT"));
        ui->aap_preludes->setChecked(options.analCmd.contains("aap"));
        ui->jmptbl->setChecked(options.analCmd.contains("e! anal.jmptbl"));
        ui->pushret->setChecked(options.analCmd.contains("e! anal.pushret"));
        ui->hasnext->setChecked(options.analCmd.contains("e! anal.hasnext"));
    }

    if (!options.script.isEmpty()) {
        ui->scriptCheckBox->setChecked(true);
        ui->scriptLineEdit->setText(options.script);
        analLevel = 0;
    } else {
        ui->scriptCheckBox->setChecked(false);
        ui->scriptLineEdit->setText("");
    }

    ui->analSlider->setValue(analLevel);

    shellcode = options.shellcode;

    // TODO: all other options should also be applied to the ui
}

QString InitialOptionsDialog::getSelectedArch()
{
    QVariant archValue = ui->archComboBox->currentData();
    return archValue.isValid() ? archValue.toString() : nullptr;
}

QString InitialOptionsDialog::getSelectedCPU()
{
    QString cpu = ui->cpuComboBox->currentText();
    if (cpu.isNull() || cpu.isEmpty())
        return nullptr;
    return cpu;
}

int InitialOptionsDialog::getSelectedBits()
{
    QString sel_bits = ui->bitsComboBox->currentText();
    if (sel_bits != "Auto") {
        return sel_bits.toInt();
    }

    return 0;
}

int InitialOptionsDialog::getSelectedBBSize()
{
    QString sel_bbsize = ui->entry_analbb->text();
    bool ok;
    int bbsize = sel_bbsize.toInt(&ok);
    if (ok)
        return bbsize;
    return 1024;
}

InitialOptions::Endianness InitialOptionsDialog::getSelectedEndianness()
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

QString InitialOptionsDialog::getSelectedOS()
{
    QVariant os = ui->kernelComboBox->currentData();
    return os.isValid() ? os.toString() : nullptr;
}

QList<QString> InitialOptionsDialog::getSelectedAdvancedAnalCmds()
{
    QList<QString> advanced = QList<QString>();
    if (ui->analSlider->value() == 3) {
        // Enable analysis configurations before executing analysis commands
        if (ui->jmptbl->isChecked()) {
            advanced << "e! anal.jmptbl";
        }
        if (ui->pushret->isChecked()) {
            advanced << "e! anal.pushret";
        }
        if (ui->hasnext->isChecked()) {
            advanced << "e! anal.hasnext";
        }
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
    }
    return advanced;
}

void InitialOptionsDialog::setupAndStartAnalysis(/*int level, QList<QString> advanced*/)
{
    main->initUI();

    InitialOptions options;

    options.filename = main->getFilename();
    if (!options.filename.isEmpty()) {
        main->setWindowTitle("Cutter – " + options.filename);
    }
    options.shellcode = this->shellcode;

    // Where the bin header is located in the file (-B)
    if (ui->entry_loadOffset->text().length() > 0) {
        options.binLoadAddr = Core()->math(ui->entry_loadOffset->text());
    }

    options.mapAddr = Core()->math(
                          ui->entry_mapOffset->text());      // Where to map the file once loaded (-m)
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

    int level = ui->analSlider->value();
    switch (level) {
    case 1:
        options.analCmd = { "aaa" };
        break;
    case 2:
        options.analCmd = { "aaaa" };
        break;
    case 3:
        options.analCmd = getSelectedAdvancedAnalCmds();
        break;
    default:
        options.analCmd = {};
        break;
    }


    AnalTask *analTask = new AnalTask();
    analTask->setOptions(options);

    MainWindow *main = this->main;
    connect(analTask, &AnalTask::openFileFailed, main, &MainWindow::openNewFileFailed);
    connect(analTask, &AsyncTask::finished, main, [analTask, main]() {
        if (analTask->getOpenFileFailed()) {
            return;
        }
        main->finalizeOpen();
    });

    AsyncTask::Ptr analTaskPtr(analTask);

    AsyncTaskDialog *taskDialog = new AsyncTaskDialog(analTaskPtr);
    taskDialog->setInterruptOnClose(true);
    taskDialog->setAttribute(Qt::WA_DeleteOnClose);
    taskDialog->show();

    Core()->getAsyncTaskManager()->start(analTaskPtr);

    done(0);
}

void InitialOptionsDialog::on_okButton_clicked()
{
    ui->okButton->setEnabled(false);
    setupAndStartAnalysis();
}

void InitialOptionsDialog::closeEvent(QCloseEvent *event)
{
    event->accept();
}

QString InitialOptionsDialog::analysisDescription(int level)
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

void InitialOptionsDialog::on_analSlider_valueChanged(int value)
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

void InitialOptionsDialog::on_AdvOptButton_clicked()
{
    if (ui->AdvOptButton->isChecked()) {
        ui->hideFrame->setVisible(true);
        ui->AdvOptButton->setArrowType(Qt::DownArrow);
    } else {
        ui->hideFrame->setVisible(false);
        ui->AdvOptButton->setArrowType(Qt::RightArrow);
    }
}

void InitialOptionsDialog::on_analCheckBox_clicked(bool checked)
{
    if (!checked) {
        analLevel = ui->analSlider->value();
    }
    ui->analSlider->setValue(checked ? analLevel : 0);
}

void InitialOptionsDialog::on_archComboBox_currentIndexChanged(int)
{
    updateCPUComboBox();
}

void InitialOptionsDialog::updatePDBLayout()
{
    ui->pdbWidget->setEnabled(ui->pdbCheckBox->isChecked());
}

void InitialOptionsDialog::on_pdbSelectButton_clicked()
{
    QFileDialog dialog(this);
    dialog.setWindowTitle(tr("Select PDB file"));
    dialog.setNameFilters({ tr("PDB file (*.pdb)"), tr("All files (*)") });

    if (!dialog.exec()) {
        return;
    }

    const QString &fileName = QDir::toNativeSeparators(dialog.selectedFiles().first());

    if (!fileName.isEmpty()) {
        ui->pdbLineEdit->setText(fileName);
    }
}


void InitialOptionsDialog::updateScriptLayout()
{
    ui->scriptWidget->setEnabled(ui->scriptCheckBox->isChecked());
}

void InitialOptionsDialog::on_scriptSelectButton_clicked()
{
    QFileDialog dialog(this);
    dialog.setWindowTitle(tr("Select radare2 script file"));
    dialog.setNameFilters({ tr("Script file (*.r2)"), tr("All files (*)") });

    if (!dialog.exec()) {
        return;
    }

    const QString &fileName = QDir::toNativeSeparators(dialog.selectedFiles().first());

    if (!fileName.isEmpty()) {
        ui->scriptLineEdit->setText(fileName);
    }
}


void InitialOptionsDialog::reject()
{
    done(0);
    NewFileDialog *n = new NewFileDialog(nullptr);
    n->show();
}
