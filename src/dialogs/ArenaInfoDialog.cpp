#include "ArenaInfoDialog.h"

#include "ui_ArenaInfoDialog.h"

ArenaInfoDialog::ArenaInfoDialog(Arena &arena, QWidget *parent)
    : QDialog(parent), ui(new Ui::ArenaInfoDialog), arena(arena)
{
    ui->setupUi(this);
    setWindowTitle("Arena @ " + rzAddressString(arena.offset));
    updateContents();
}

void ArenaInfoDialog::updateContents()
{
    ui->lineEditTop->setText(rzAddressString(arena.top));
    ui->lineEditLastRem->setText(rzAddressString(arena.lastRemainder));
    ui->lineEditNext->setText(rzAddressString(arena.next));
    ui->lineEditNextfree->setText(rzAddressString(arena.nextFree));
    ui->lineEditSysMem->setText(rzAddressString(arena.systemMem));
    ui->lineEditMaxMem->setText(rzAddressString(arena.maxSystemMem));
}

ArenaInfoDialog::~ArenaInfoDialog() {}
