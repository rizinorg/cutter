#include "ArenaInfoDialog.h"
#include "ui_ArenaInfoDialog.h"

ArenaInfoDialog::ArenaInfoDialog(Arena &arena, QWidget *parent)
    : arena(arena), QDialog(parent), ui(new Ui::ArenaInfoDialog)
{
    ui->setupUi(this);
    setWindowTitle("Arena @ " + RAddressString(arena.offset));
    updateContents();
}

void ArenaInfoDialog::updateContents()
{
    ui->lineEditTop->setText(RAddressString(arena.top));
    ui->lineEditLastRem->setText(RAddressString(arena.last_remainder));
    ui->lineEditNext->setText(RAddressString(arena.next));
    ui->lineEditNextfree->setText(RAddressString(arena.next_free));
    ui->lineEditSysMem->setText(RAddressString(arena.system_mem));
    ui->lineEditMaxMem->setText(RAddressString(arena.max_system_mem));
}

ArenaInfoDialog::~ArenaInfoDialog()
{
    delete ui;
}
