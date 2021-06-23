#include "HeapInfoDialog.h"

#include <utility>
#include "ui_HeapInfoDialog.h"

HeapInfoDialog::HeapInfoDialog(RVA offset, QString status, QWidget *parent)
    : QDialog(parent), ui(new Ui::HeapInfoDialog), offset(offset), status(std::move(status))
{
    ui->setupUi(this);
    this->ui->rbIM->setAttribute(Qt::WA_TransparentForMouseEvents);
    this->ui->rbIM->setFocusPolicy(Qt::NoFocus);
    this->ui->rbNMA->setAttribute(Qt::WA_TransparentForMouseEvents);
    this->ui->rbNMA->setFocusPolicy(Qt::NoFocus);
    this->ui->rbPI->setAttribute(Qt::WA_TransparentForMouseEvents);
    this->ui->rbPI->setFocusPolicy(Qt::NoFocus);
    updateFields();
}

HeapInfoDialog::~HeapInfoDialog()
{
    delete ui;
}

void HeapInfoDialog::updateFields()
{
    this->setWindowTitle(QString("Chunk @ ") + RAddressString(offset)
                         + QString("(" + status + ")"));
    this->ui->baseEdit->setText(RAddressString(offset));
    RzHeapChunkSimple *chunk = Core()->getHeapChunk(offset);
    if (!chunk) {
        return;
    }
    this->ui->sizeEdit->setText(RHexString(chunk->size));
    this->ui->bkEdit->setText(RAddressString(chunk->bk));
    this->ui->fdEdit->setText(RAddressString(chunk->fd));
    this->ui->bknsEdit->setText(RAddressString(chunk->bk_nextsize));
    this->ui->fdnsEdit->setText(RAddressString(chunk->fd_nextsize));
    this->ui->prevSizeEdit->setText(RHexString(chunk->prev_size));
    if (chunk->is_mmapped) {
        this->ui->rbIM->setChecked(true);
    }
    if (chunk->prev_inuse) {
        this->ui->rbPI->setChecked(true);
    }
    if (chunk->non_main_arena) {
        this->ui->rbNMA->setChecked(true);
    }
    free(chunk);
}
