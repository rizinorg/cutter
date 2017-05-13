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


OptionsDialog::OptionsDialog(MainWindow *main):
    QDialog(0), // parent may not be main
    ui(new Ui::OptionsDialog),
    analThread(this),
    main(main),
    defaultAnalLevel(3)
{
    ui->setupUi(this);
    setWindowFlags(windowFlags() & (~Qt::WindowContextHelpButtonHint));
    ui->progressBar->setVisible(0);
    ui->statusLabel->setVisible(0);

    ui->analSlider->setValue(defaultAnalLevel);

    // Fill the plugins combo
    asm_plugins = main->core->getAsmPluginNames();
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

    ui->programLineEdit->setText(main->getFilename());
    this->main->core->tryFile(main->getFilename(), true);
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

    int va = ui->vaCheckBox->isChecked();
    ut64 loadaddr = 0LL;
    ut64 mapaddr = 0LL;

    // Save options in settings
    Settings settings;
    settings.setAsmBytes(ui->bytesCheckBox->isChecked());
    settings.setATnTSyntax(ui->attCheckBox->isChecked());
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
    QVariant archValue = ui->processorComboBox->currentData();

    int bits = 0;
    QString sel_bits = ui->bitsComboBox->currentText();
    if (sel_bits != "Auto")
    {
        bits = sel_bits.toInt();
    }

    main->core->setCPU(archValue.isValid() ? archValue.toString() : NULL,
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
    ui->statusLabel->setText("Loading binary");
    // options dialog should show the list of archs inside the given fatbin
    int binidx = 0; // index of subbin

    main->addOutput(" > Loading file: " + main->getFilename());
    main->core->loadFile(main->getFilename(), loadaddr, mapaddr, rw, va, binidx, load_bininfo);
    //ui->progressBar->setValue(40);
    ui->statusLabel->setText("Analysis in progress");

    // Threads stuff
    // connect signal/slot

    analThread.start(main->core, level);
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
    ui->statusLabel->setText("Loading interface");
    main->addOutput(" > Analysis finished");

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
