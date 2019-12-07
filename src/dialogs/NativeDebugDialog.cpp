#include "NativeDebugDialog.h"
#include "ui_NativeDebugDialog.h"

#include <QHostAddress>
#include <QFileInfo>
#include <QMessageBox>

#define GDBSERVER "GDB"
#define WINDBGPIPE "WinDbg - Pipe"
#define WINDBG_URI_PREFIX "windbg"
#define GDB_URI_PREFIX "gdb"
#define DEFAULT_INDEX (GDBSERVER)

NativeDebugDialog::NativeDebugDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::NativeDebugDialog)
{
    ui->setupUi(this);
    setWindowFlags(windowFlags() & (~Qt::WindowContextHelpButtonHint));
}

NativeDebugDialog::~NativeDebugDialog() {}

void NativeDebugDialog::on_buttonBox_accepted()
{
}

void NativeDebugDialog::on_buttonBox_rejected()
{
    close();
}

QString NativeDebugDialog::getArgs() const
{
    return ui->argEdit->text();
}
