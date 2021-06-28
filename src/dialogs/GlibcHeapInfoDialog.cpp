#include "GlibcHeapInfoDialog.h"

#include <utility>
#include "ui_GlibcHeapInfoDialog.h"

GlibcHeapInfoDialog::GlibcHeapInfoDialog(RVA offset, QString status, QWidget *parent)
    : QDialog(parent), ui(new Ui::GlibcHeapInfoDialog), offset(offset), status(std::move(status))
{
    ui->setupUi(this);

    // disable all the radio buttons for flag field so they are not user editable
    this->ui->rbIM->setEnabled(false);
    this->ui->rbNMA->setEnabled(false);
    this->ui->rbPI->setEnabled(false);

    // set window title
    QString windowTitle = tr("Chunk @ ") + RAddressString(offset);
    if (!this->status.isEmpty()) {
        windowTitle += QString("(" + this->status + ")");
    }
    this->setWindowTitle(windowTitle);

    updateFields();
}

GlibcHeapInfoDialog::~GlibcHeapInfoDialog()
{
    delete ui;
}

void GlibcHeapInfoDialog::updateFields()
{
    // get data about the heap chunk from the API
    RzHeapChunkSimple *chunk = Core()->getHeapChunk(offset);
    if (!chunk) {
        return;
    }

    // Update the information in the widget
    this->ui->baseEdit->setText(RAddressString(offset));
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
