#include "optionsdialog.h"
#include "ui_optionsdialog.h"
#include "mainwindow.h"
#include "newfiledialog.h"
#include "helpers.h"

// TODO: remove us
#include "widgets/memorywidget.h"
#include "widgets/notepad.h"

#include <QSettings>


OptionsDialog::OptionsDialog(const QString &filename, QWidget *parent):
    QDialog(parent),
    ui(new Ui::OptionsDialog),
    core(new QRCore()),
    analThread(this),
    w(nullptr),
    filename(filename),
    defaultAnalLevel(3)
{
    ui->setupUi(this);
    setWindowFlags(windowFlags() & (~Qt::WindowContextHelpButtonHint));
    ui->progressBar->setVisible(0);
    ui->statusLabel->setVisible(0);

    ui->analSlider->setValue(defaultAnalLevel);

    // Fill the plugins combo
    asm_plugins = core->getAsmPluginNames();
    for (auto plugin : asm_plugins)
        ui->processorComboBox->addItem(plugin, plugin);

    // Restore settings
    QSettings settings;
    ui->bytesCheckBox->setChecked(settings.value("bytes").toBool());
    ui->attCheckBox->setChecked(settings.value("syntax").toBool());
    ui->descriptionCheckBox->setChecked(settings.value("describe").toBool());
    ui->stackCheckBox->setChecked(settings.value("stackptr").toBool());
    ui->ucaseCheckBox->setChecked(settings.value("ucase").toBool());
    ui->spacyCheckBox->setChecked(settings.value("spacy").toBool());

    ui->hideFrame->setVisible(false);

    // Add this so the dialog resizes when widgets are shown/hidden
    //this->layout()->setSizeConstraint(QLayout::SetFixedSize);

    connect(&analThread, SIGNAL(finished()), this, SLOT(anal_finished()));

    ui->programLineEdit->setText(filename);
    this->core->tryFile(filename, true);
}

OptionsDialog::~OptionsDialog()
{
    delete ui;
}

void OptionsDialog::setupAndStartAnalysis(int level)
{
    ui->analSlider->setValue(level);

    this->setEnabled(0);
    ui->logo->setEnabled(true);

    // Show Progress Bar
    ui->progressBar->setEnabled(1);
    ui->statusLabel->setEnabled(1);
    ui->progressBar->setVisible(1);
    ui->statusLabel->setVisible(1);

    ui->statusLabel->setText("Starting analysis");
    //ui->progressBar->setValue(5);

    // Close dialog and open OptionsDialog
    this->w = new MainWindow(0, this->core);

    // Fill asm plugins in hexdump combo
    this->w->memoryDock->fillPlugins(this->asm_plugins);

    int va = ui->vaCheckBox->isChecked();
    ut64 loadaddr = 0LL;
    ut64 mapaddr = 0LL;

    // Save options in settings
    QSettings settings;

    // Show asm bytes
    if (ui->bytesCheckBox->isChecked())
    {
        this->w->core->config("asm.bytes", "true");
        this->w->core->config("asm.cmtcol", "100");
    }
    else
    {
        this->w->core->config("asm.bytes", "false");
        this->w->core->config("asm.cmtcol", "70");
    }
    settings.setValue("bytes", ui->bytesCheckBox->isChecked());

    // Show AT&T syntax
    if (ui->attCheckBox->isChecked())
    {
        this->w->core->config("asm.syntax", "att");
    }
    else
    {
        this->w->core->config("asm.syntax", "intel");
    }
    settings.setValue("syntax", ui->attCheckBox->isChecked());

    // Show opcode description
    if (ui->descriptionCheckBox->isChecked())
    {
        this->w->core->config("asm.describe", "true");
    }
    else
    {
        this->w->core->config("asm.describe", "false");
    }
    settings.setValue("describe", ui->descriptionCheckBox->isChecked());

    // Show stack pointer
    if (ui->stackCheckBox->isChecked())
    {
        this->w->core->config("asm.stackptr", "true");
    }
    else
    {
        this->w->core->config("asm.stackptr", "false");
    }
    settings.setValue("stackptr", ui->stackCheckBox->isChecked());

    // Show uppercase dasm
    if (ui->ucaseCheckBox->isChecked())
    {
        this->w->core->config("asm.ucase", "true");
    }
    else
    {
        this->w->core->config("asm.ucase", "false");
    }
    settings.setValue("ucase", ui->ucaseCheckBox->isChecked());

    // Show spaces in dasm
    if (ui->spacyCheckBox->isChecked())
    {
        this->w->core->config("asm.spacy", "true");
    }
    else
    {
        this->w->core->config("asm.spacy", "false");
    }
    settings.setValue("spacy", ui->spacyCheckBox->isChecked());



    //
    // Advanced Options
    //
    QVariant archValue = ui->processorComboBox->currentData();

    int bits = 0;
    QString sel_bits = ui->bitsComboBox->currentText();
    if (sel_bits != "Auto")
    {
        bits = sel_bits.toInt();
    }

    w->core->setCPU(archValue.isValid() ? archValue.toString() : NULL,
                    QString(),
                    bits);



    bool rw = false;
    bool load_bininfo = ui->binCheckBox->isChecked();

    if (load_bininfo)
    {
        if (!va)
        {
            va = 2;
            loadaddr = UT64_MAX;
            r_config_set_i(this->core->core()->config, "bin.laddr", loadaddr);
            mapaddr = 0;
        }
    }
    else
    {
        va = false;
        loadaddr = mapaddr = 0;
    }

    //ui->progressBar->setValue(20);
    ui->statusLabel->setText("Loading binary");
    // options dialog should show the list of archs inside the given fatbin
    int binidx = 0; // index of subbin

    this->w->addOutput(" > Loading file: " + this->filename);
    this->w->core->loadFile(this->filename, loadaddr, mapaddr, rw, va, binidx, load_bininfo);
    //ui->progressBar->setValue(40);
    ui->statusLabel->setText("Analysis in progress");

    // Threads stuff
    // connect signal/slot

    analThread.start(core, level);
}

void OptionsDialog::on_closeButton_clicked()
{
    close();
}

void OptionsDialog::on_okButton_clicked()
{
    setupAndStartAnalysis(ui->analSlider->value());
}

void OptionsDialog::anal_finished()
{
    // Get opcodes
    this->w->core->getOpcodes();

    //fprintf(stderr, "anal done");
    //ui->progressBar->setValue(70);

    const QString uniqueName(qhelpers::uniqueProjectName(filename));

    this->w->core->cmd("Po " + uniqueName);
    // Set settings to override any incorrect saved in the project
    this->core->setSettings();
    ui->statusLabel->setText("Loading interface");
    this->w->addOutput(" > Analysis finished");
    QString initial_seek = ui->entry_initialSeek->text();
    if (initial_seek.length() > 0)
    {
        this->w->core->seek(initial_seek);
    }
    else
    {
        this->w->core->seek("entry0");
    }
    this->w->addOutput(" > Populating UI");
    // FIXME: initialization order frakup. the next line is needed so that the
    // comments widget displays the function names.
    core->cmd("fs sections");
    this->w->updateFrames();
    this->w->setFilename(this->filename);
    this->w->get_refs(this->w->core->cmd("?v entry0"));
    this->w->memoryDock->selectHexPreview();

    // Restore project notes
    QString notes = this->core->cmd("Pn");
    //qDebug() << "Notes:" << notes;
    if (notes != "")
    {
        QByteArray ba;
        ba.append(notes);
        this->w->notepadDock->setText(QByteArray::fromBase64(ba));
    }

    //Get binary beginning/end addresses
    this->core->binStart = this->core->cmd("?v $M");
    this->core->binEnd = this->core->cmd("?v $M+$s");

    this->w->addOutput(" > Finished, happy reversing :)");
    // Add fortune message
    this->w->addOutput("\n" + this->w->core->cmd("fo"));
    this->w->memoryDock->setWindowTitle("entry0");
    this->w->start_web_server();
    close();
    this->w->showMaximized();
    // Initialize syntax highlighters
    this->w->memoryDock->highlightDisasms();
    this->w->notepadDock->highlightPreview();
}

void OptionsDialog::on_cancelButton_clicked()
{
    delete this->core;
    this->core = NULL;
    // Close dialog and open OptionsDialog
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
        return tr("-");
    case 1:
        return tr("Minimum");
    case 2:
        return tr("Basic");
    case 3:
        return tr("Medium");
    case 4:
        return tr("Full <font color='red'><b>(Experimental)</b></font>");
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
        ui->analCheckBox->setText("Analysis: Disabled");
    }
    else
    {
        ui->analCheckBox->setChecked(true);
        ui->analCheckBox->setText("Analysis: Enabled");
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
