#include "createnewdialog.h"
#include "ui_createnewdialog.h"
#include <QMessageBox>
#include "newfiledialog.h"
#include "r_util.h"

createNewDialog::createNewDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::createNewDialog)
{
    ui->setupUi(this);
    w = new MainWindow(this);
    w->core = new QRCore ();
}

createNewDialog::~createNewDialog()
{
    delete ui;
}

void createNewDialog::on_pushButton_2_clicked()
{
    // Close dialog and open OptionsDialog
    close();
    NewFileDialog* n = new NewFileDialog(this);
    n->show();
}

void createNewDialog::on_pushButton_3_clicked()
{
    close();
}

void createNewDialog::on_exampleButton_clicked()
{
    QString type = ui->comboType->currentText();
    QString str;
    if (type == "Assembler") {
        str = "; Sample program code\nmov eax, 1\nint 0x80";
    } else if (type == "Text") {
        str = "Hello World";
    } else if (type == "Rapatch") {
        str = "; Sample rapatch script\n"
                "0x0 \"Hello World\n"
                "0x10 909090";
    } else if (type == "C Code") {
        str = "int main() {\n"
                "  write (1, \"Hello World\", 12);\n"
                "  exit (0);\n"
                "}";
    } else if (type == "Radare2 script") {
        str = "w Hello\ns+5\nw World";
    } else if (type == "Hexpairs") {
        str = "48656c6c6f20576f726c6400";
    } else fprintf (stderr, "Unknown combo value selected");
    if (str.length()>0)
        ui->plainTextEdit->setPlainText(str);
    // }
}

void createNewDialog::on_buttonCreate_clicked()
{
    QString type = ui->comboType->currentText();
    QString str;
    bool created = false;

    QString arch = ui->comboArch->currentText();
    int bits = atoi (ui->comboBits->currentText().toStdString().c_str());
    int fsize = r_num_math (NULL, ui->entrySize->text().toStdString().c_str());
    QString format = ui->comboFormat->currentText();

    if (type == "Assembler") {
        const char *asmcode = ui->plainTextEdit->toPlainText().toStdString().c_str();
        RAsmCode *code = r_asm_massemble (w->core->core->assembler, asmcode);
        if (code && code->len>0) {
            char file[32];
            snprintf (file, sizeof(file)-1, "malloc://%d", code->len);
            if (w->core->loadFile(file,0,0,1,0,0,false)) {
                created = true;
                r_core_write_at(w->core->core,0, code->buf, code->len);
            } else {
                __alert ("Failed to create file");
            }
        } else {
            __alert ("Invalid assembler code");
        }
        r_asm_code_free (code);
    } else if (type == "Rapatch") {
        if (fsize>0) {
            char file[32];
            created = true;
            snprintf (file, sizeof(file)-1, "malloc://%d", fsize);
            if (w->core->loadFile(file,0,0,1,0,0,false)) {
                const char *rapatch = ui->plainTextEdit->toPlainText().toStdString().c_str();
                r_core_patch (w->core->core, rapatch);
                r_core_seek(w->core->core, 0, 1);
                created = true;
            } else {
                __alert ("failed to open file");
            }
        } else {
            __alert ("Invalid file size");
        }
    } else if (type == "C Code") {
        __alert("C Code: TODO");
        // ragg2-cc -x
    } else if (type == "Radare2 script") {
        if (fsize>0) {
            char file[32];
            created = true;
            snprintf (file, sizeof(file)-1, "malloc://%d", fsize);
            if (w->core->loadFile(file,0,0,1,0,0,false)) {
                created = true;
                QString str = ui->plainTextEdit->toPlainText().toStdString().c_str();
                QList <QString> lines = str.split("\n");
                foreach (QString str, lines) {
                    w->core->cmd(str);
                }
            } else {
                __alert ("failed to open file");
            }
        } else {
            __alert ("Invalid file size");
        }
    } else if (type == "Text") {
        char file[32];
        const char *hexpairs = ui->plainTextEdit->toPlainText().toStdString().c_str();
        int sz = strlen (hexpairs);
        if (sz>0) {
            snprintf (file, sizeof(file)-1, "malloc://%d", sz);
            if (w->core->loadFile(file,0,0,1,0,0,false)) {
                created = true;
                r_core_write_at(w->core->core,0, (const ut8*)hexpairs, sz);
            } else {
                __alert ("failed to open file");
            }
        } else {
            __alert ("Empty string?");
        }
    } else if (type == "Hexpairs") {
        char file[32];
        int sz;
        const char *hexpairs = ui->plainTextEdit->toPlainText().toStdString().c_str();
        ut8 *buf = (ut8*)malloc (strlen (hexpairs));
        sz = r_hex_str2bin (hexpairs, buf);
        if (sz>0) {
            snprintf (file, sizeof(file)-1, "malloc://%d", sz);
            if (w->core->loadFile(file,0,0,1,0,0,false)) {
                created = true;
                r_core_write_at(w->core->core,0, buf, sz);
            } else {
                __alert ("failed to open file");
            }
        } else {
            __alert ("Invalid hexpair string");
        }
        free (buf);
    } else {
        __alert ("Unknown combo value selected");
        return;
    }

    if (format != "Raw") {
        __alert ("TODO: non-raw fileformat is not yet supported");
        created = false;
        delete w->core;
    }

    if (created) {

        // Close dialog and open OptionsDialog
        close();

        w->core->seek(0);
        w->updateFrames();
        w->setFilename("-");
        w->add_output("Finished, check its contents");
        w->showMaximized();
    } else {
        __alert ("No file created.");

    }
}
