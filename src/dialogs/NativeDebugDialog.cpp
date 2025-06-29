#include "NativeDebugDialog.h"
#include "ui_NativeDebugDialog.h"
#include "shortcuts/ShortcutManager.h"

#include <QMessageBox>
#include <QShortcut>

NativeDebugDialog::NativeDebugDialog(QWidget *parent)
    : QDialog(parent), ui(new Ui::NativeDebugDialog)
{
    ui->setupUi(this);
    setWindowFlags(windowFlags() & (~Qt::WindowContextHelpButtonHint));
    auto shortcut = Shortcuts()->makeQShortcut("Debug.accept", ui->argEdit);
    shortcut->setContext(Qt::ShortcutContext::WidgetShortcut);
    connect(shortcut, &QShortcut::activated, this, &QDialog::accept);
}

NativeDebugDialog::~NativeDebugDialog() {}

QString NativeDebugDialog::getArgs() const
{
    return ui->argEdit->toPlainText();
}

void NativeDebugDialog::setArgs(const QString &args)
{
    ui->argEdit->setPlainText(args);
    ui->argEdit->selectAll();
}
