#include "ArenaInfoDialog.h"
#include "ui_ArenaInfoDialog.h"

ArenaInfoDialog::ArenaInfoDialog(Arena &arena, QWidget *parent)
    : QDialog(parent), ui(new Ui::ArenaInfoDialog), arena(arena)
{
    ui->setupUi(this);
    setWindowTitle("Arena @ " + RzAddressString(arena.offset));
    updateContents();
}

void ArenaInfoDialog::updateContents()
{
    ui->lineEditTop->setText(RzAddressString(arena.top));
    ui->lineEditLastRem->setText(RzAddressString(arena.last_remainder));
    ui->lineEditNext->setText(RzAddressString(arena.next));
    ui->lineEditNextfree->setText(RzAddressString(arena.next_free));
    ui->lineEditSysMem->setText(RzAddressString(arena.system_mem));
    ui->lineEditMaxMem->setText(RzAddressString(arena.max_system_mem));
}

ArenaInfoDialog::~ArenaInfoDialog()
{
    delete ui;
}
