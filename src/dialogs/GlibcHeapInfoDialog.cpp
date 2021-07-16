#include "GlibcHeapInfoDialog.h"

#include <utility>
#include "ui_GlibcHeapInfoDialog.h"

GlibcHeapInfoDialog::GlibcHeapInfoDialog(RVA offset, QString status, QWidget *parent)
    : QDialog(parent), ui(new Ui::GlibcHeapInfoDialog), offset(offset), status(std::move(status))
{
    ui->setupUi(this);

    // set window title
    QString windowTitle = tr("Chunk @ ") + RAddressString(offset);
    if (!this->status.isEmpty()) {
        windowTitle += QString("(" + this->status + ")");
    }
    this->setWindowTitle(windowTitle);

    connect(ui->saveButton, &QPushButton::clicked, this, &GlibcHeapInfoDialog::saveChunkInfo);

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
    } else {
        this->ui->rbIM->setChecked(false);
    }
    if (chunk->prev_inuse) {
        this->ui->rbPI->setChecked(true);
    } else {
        this->ui->rbPI->setChecked(false);
    }
    if (chunk->non_main_arena) {
        this->ui->rbNMA->setChecked(true);
    } else {
        this->ui->rbNMA->setChecked(false);
    }

    free(chunk);
}

void GlibcHeapInfoDialog::saveChunkInfo()
{
    QMessageBox msgBox;
    msgBox.setText("Do you want to overwrite chunk metadata?");
    msgBox.setInformativeText(
            "Any field which cannot be converted to a valid integer will be saved as zero");
    msgBox.setStandardButtons(QMessageBox::Save | QMessageBox::Cancel);
    msgBox.setDefaultButton(QMessageBox::Save);

    int ret = msgBox.exec();
    switch (ret) {
    case QMessageBox::Save:
        // Save was clicked
        RzHeapChunkSimple chunkSimple;
        chunkSimple.size = Core()->math(ui->sizeEdit->text());
        chunkSimple.fd = Core()->math(ui->fdEdit->text());
        chunkSimple.bk = Core()->math(ui->bkEdit->text());
        chunkSimple.fd_nextsize = Core()->math(ui->fdnsEdit->text());
        chunkSimple.bk_nextsize = Core()->math(ui->bknsEdit->text());
        chunkSimple.addr = offset;
        if (ui->rbIM->isChecked()) {
            chunkSimple.is_mmapped = true;
        } else {
            chunkSimple.is_mmapped = false;
        }
        if (ui->rbNMA->isChecked()) {
            chunkSimple.non_main_arena = true;
        } else {
            chunkSimple.non_main_arena = false;
        }
        if (ui->rbPI->isChecked()) {
            chunkSimple.prev_inuse = true;
        } else {
            chunkSimple.prev_inuse = false;
        }
        if (Core()->writeHeapChunk(&chunkSimple)) {
            updateFields();
            QMessageBox::information(this, tr("Chunk saved"),
                                     tr("Chunk header successfully overwritten"));
        } else {
            QMessageBox::information(this, tr("Chunk not saved"),
                                     tr("Chunk header not successfully overwritten"));
        }
        break;
    case QMessageBox::Cancel:
        // Cancel was clicked
        break;
    }
}