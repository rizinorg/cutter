#include <QMessageBox>
#include "dialogs/NewFileDialog.h"
#include "dialogs/CreateNewDialog.h"
#include "ui_CreateNewDialog.h"
#include "r_util.h"

CreateNewDialog::CreateNewDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::CreateNewDialog),
    w(new MainWindow),
    core(CutterCore::getInstance())
{
    ui->setupUi(this);
    setWindowFlags(windowFlags() & (~Qt::WindowContextHelpButtonHint));
}

CreateNewDialog::~CreateNewDialog() {}

void CreateNewDialog::on_pushButton_2_clicked()
{
    // Close dialog and open OptionsDialog
    close();
    NewFileDialog *n = new NewFileDialog(nullptr); // TODO: This leaks
    n->show();
}

void CreateNewDialog::on_pushButton_3_clicked()
{
    close();
}

void CreateNewDialog::on_exampleButton_clicked()
{
    QString type = ui->comboType->currentText();
    QString str;
    if (type == "Assembler")
    {
        str = "; Sample program code\nmov eax, 1\nint 0x80";
    }
    else if (type == "Text")
    {
        str = "Hello World";
    }
    else if (type == "Rapatch")
    {
        str = "; Sample rapatch script\n"
              "0x0 \"Hello World\n"
              "0x10 909090";
    }
    else if (type == "C Code")
    {
        str = "int main() {\n"
              "  write (1, \"Hello World\", 12);\n"
              "  exit (0);\n"
              "}";
    }
    else if (type == "Radare2 script")
    {
        str = "w Hello\ns+5\nw World";
    }
    else if (type == "Hexpairs")
    {
        str = "48656c6c6f20576f726c6400";
    }
    else fprintf(stderr, "%s", tr("Unknown combo value selected").toLocal8Bit().constData());
    if (str.length() > 0)
        ui->plainTextEdit->setPlainText(str);
    // }
}

void CreateNewDialog::on_buttonCreate_clicked()
{
    RCoreLocked lcore = core->core();
    QString type = ui->comboType->currentText();
    QString str;
    bool created = false;

    QString arch = ui->comboArch->currentText();
    int fsize = r_num_math(NULL, ui->entrySize->text().toUtf8().constData());
    QString format = ui->comboFormat->currentText();

    if (type == "Assembler")
    {
        RAsmCode *code = r_asm_massemble(lcore->assembler, ui->plainTextEdit->toPlainText().toUtf8().constData());
        if (code && code->len > 0)
        {
            char file[32];
            snprintf(file, sizeof(file) - 1, "malloc://%d", code->len);
            if (core->loadFile(file, 0, 0, 1, 0, 0, false))
            {
                created = true;
                r_core_write_at(lcore, 0, code->buf, code->len);
            }
            else
            {
                __alert(tr("Failed to create file"));
            }
        }
        else
        {
            __alert(tr("Invalid assembler code"));
        }
        r_asm_code_free(code);
    }
    else if (type == "Rapatch")
    {
        if (fsize > 0)
        {
            char file[32];
            created = true;
            snprintf(file, sizeof(file) - 1, "malloc://%d", fsize);
            if (core->loadFile(file, 0, 0, 1, 0, 0, false))
            {
                r_core_patch(lcore, ui->plainTextEdit->toPlainText().toUtf8().constData());
                r_core_seek(lcore, 0, 1);
                created = true;
            }
            else
            {
                __alert(tr("Failed to open file"));
            }
        }
        else
        {
            __alert(tr("Invalid file size"));
        }
    }
    else if (type == "C Code")
    {
        __alert("C Code: TODO");
        // ragg2-cc -x
    }
    else if (type == "Radare2 script")
    {
        if (fsize > 0)
        {
            char file[32];
            created = true;
            snprintf(file, sizeof(file) - 1, "malloc://%d", fsize);
            if (core->loadFile(file, 0, 0, 1, 0, 0, false))
            {
                created = true;
                QString str = ui->plainTextEdit->toPlainText();
                QList <QString> lines = str.split("\n");
                foreach (QString str, lines)
                {
                    core->cmd(str);
                }
            }
            else
            {
                __alert(tr("Failed to open file"));
            }
        }
        else
        {
            __alert(tr("Invalid file size"));
        }
    }
    else if (type == "Text")
    {
        char file[32];
        QByteArray hexpairs = ui->plainTextEdit->toPlainText().toUtf8();
        size_t sz = strlen(hexpairs.constData());
        if (sz > 0)
        {
            snprintf(file, sizeof(file) - 1, "malloc://%d", (int)sz);
            if (core->loadFile(file, 0, 0, 1, 0, 0, false))
            {
                created = true;
                r_core_write_at(lcore, 0, (const ut8 *)hexpairs.constData(), (int)sz);
            }
            else
            {
                __alert(tr("Failed to open file"));
            }
        }
        else
        {
            __alert(tr("Empty string?"));
        }
    }
    else if (type == "Hexpairs")
    {
        char file[32];
        int sz;
        QByteArray hexpairs = ui->plainTextEdit->toPlainText().toUtf8();
        ut8 *buf = (ut8 *)malloc(strlen(hexpairs.constData()) + 1);
        sz = r_hex_str2bin(hexpairs.constData(), buf);
        if (sz > 0)
        {
            snprintf(file, sizeof(file) - 1, "malloc://%d", sz);
            if (core->loadFile(file, 0, 0, 1, 0, 0, false))
            {
                created = true;
                r_core_write_at(lcore, 0, buf, sz);
            }
            else
            {
                __alert(tr("Failed to open file"));
            }
        }
        else
        {
            __alert(tr("Invalid hexpair string"));
        }
        free(buf);
    }
    else
    {
        __alert(tr("Unknown combo value selected"));
        return;
    }

    if (format != "Raw")
    {
        __alert("TODO: non-raw fileformat is not yet supported");
        created = false;
        delete core;
        core = nullptr;
    }

    if (created)
    {

        // Close dialog and open OptionsDialog
        close();

        core->seek(0);
        w->updateFrames();
        w->setFilename("-");
        w->addOutput(tr("Finished, check its contents"));
        w->showMaximized();
    }
    else
    {
        __alert(tr("No file created."));

    }
}
