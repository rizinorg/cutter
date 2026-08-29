#include "HexDiffWidget.h"

#include "ui_HexDiffWidget.h"

#include <QClipboard>

HexDiffWidget::HexDiffWidget(CutterDiff *cutterDiff, CutterDiffWindow *parent)
    : CutterDiffWidget(cutterDiff, parent),
      ui(new Ui::HexDiffWidget),
      hexDiffView(new HexDiffView(this->cutterDiff, this))
{
    ui->setupUi(this);

    ui->splitter->insertWidget(0, hexDiffView);
    ui->splitter->setSizes({ 750, 250 });

    connect(ui->shiftUpA, &QPushButton::clicked, this, [this]() { hexDiffView->transpose(-1, 0); });
    connect(ui->shiftDownA, &QPushButton::clicked, this,
            [this]() { hexDiffView->transpose(1, 0); });
    connect(ui->shiftUpB, &QPushButton::clicked, this, [this]() { hexDiffView->transpose(0, -1); });
    connect(ui->shiftDownB, &QPushButton::clicked, this,
            [this]() { hexDiffView->transpose(0, 1); });
    ui->tabParsing->show();

    // Parsing
    // Info

    ui->copyMD5A->setIcon(QIcon(":/img/icons/copy.svg"));
    ui->copySHA1A->setIcon(QIcon(":/img/icons/copy.svg"));
    ui->copySHA256A->setIcon(QIcon(":/img/icons/copy.svg"));
    ui->copyCRC32A->setIcon(QIcon(":/img/icons/copy.svg"));

    ui->copyMD5B->setIcon(QIcon(":/img/icons/copy.svg"));
    ui->copySHA1B->setIcon(QIcon(":/img/icons/copy.svg"));
    ui->copySHA256B->setIcon(QIcon(":/img/icons/copy.svg"));
    ui->copyCRC32B->setIcon(QIcon(":/img/icons/copy.svg"));

    // Setup Placeholders
    const QString placeholder = tr("Select bytes to display information");

    ui->bytesMD5A->setPlaceholderText(placeholder);
    ui->bytesEntropyA->setPlaceholderText(placeholder);
    ui->bytesSHA1A->setPlaceholderText(placeholder);
    ui->bytesSHA256A->setPlaceholderText(placeholder);
    ui->bytesCRC32A->setPlaceholderText(placeholder);

    ui->bytesMD5B->setPlaceholderText(placeholder);
    ui->bytesEntropyB->setPlaceholderText(placeholder);
    ui->bytesSHA1B->setPlaceholderText(placeholder);
    ui->bytesSHA256B->setPlaceholderText(placeholder);
    ui->bytesCRC32B->setPlaceholderText(placeholder);

    // HexDiffView signals
    connect(hexDiffView, &HexDiffView::selectionChanged, this, &HexDiffWidget::selectionChanged);

    // Copy buttons - File A
    connect(ui->copyMD5A, &QPushButton::clicked, this, &HexDiffWidget::onCopyMD5AClicked);

    connect(ui->copySHA1A, &QPushButton::clicked, this, &HexDiffWidget::onCopyShA1AClicked);

    connect(ui->copySHA256A, &QPushButton::clicked, this, &HexDiffWidget::onCopyShA256AClicked);

    connect(ui->copyCRC32A, &QPushButton::clicked, this, &HexDiffWidget::onCopyCrC32AClicked);

    // Copy buttons - File B
    connect(ui->copyMD5B, &QPushButton::clicked, this, &HexDiffWidget::onCopyMD5BClicked);

    connect(ui->copySHA1B, &QPushButton::clicked, this, &HexDiffWidget::onCopyShA1BClicked);

    connect(ui->copySHA256B, &QPushButton::clicked, this, &HexDiffWidget::onCopyShA256BClicked);

    connect(ui->copyCRC32B, &QPushButton::clicked, this, &HexDiffWidget::onCopyCrC32BClicked);

    // Hex diff selection
    connect(hexDiffView, &HexDiffView::selectionChanged, this, &HexDiffWidget::selectionChanged);
    // Connect for reload
    reload();
}

HexDiffWidget::~HexDiffWidget()
{
    delete ui;
}

void HexDiffWidget::seek(QPair<RVA, RVA> addr)
{
    if (addr.first == RVA_INVALID && addr.second == RVA_INVALID) {
        return;
    }

    if (addr.second == RVA_INVALID) {
        hexDiffView->seek(addr.first, true);
    } else if (addr.first == RVA_INVALID) {
        hexDiffView->seek(addr.second, false);
    } else {
        const int transpose = addr.first > addr.second ? -static_cast<int>(addr.first - addr.second)
                                                       : static_cast<int>(addr.second - addr.first);
        hexDiffView->transpose(0, transpose, true);
        hexDiffView->seek(addr.first);
    }
}

void HexDiffWidget::reload()
{
    const QFont font = Config()->getFont();
    hexDiffView->setMonospaceFont(font);
    hexDiffView->refresh();
}

void HexDiffWidget::onCopyMD5AClicked()
{
    const QString md5A = ui->bytesMD5A->text();
    QApplication::clipboard()->setText(md5A);
}

void HexDiffWidget::onCopyShA1AClicked()
{
    const QString sha1A = ui->bytesSHA1A->text();
    QApplication::clipboard()->setText(sha1A);
}

void HexDiffWidget::onCopyShA256AClicked()
{

    const QString sha256A = ui->bytesSHA256A->text();
    QApplication::clipboard()->setText(sha256A);
}

void HexDiffWidget::onCopyCrC32AClicked()
{
    const QString crc32A = ui->bytesCRC32A->text();
    QApplication::clipboard()->setText(crc32A);
}

void HexDiffWidget::onCopyMD5BClicked()
{
    const QString md5B = ui->bytesMD5B->text();
    QApplication::clipboard()->setText(md5B);
}

void HexDiffWidget::onCopyShA1BClicked()
{
    const QString sha1B = ui->bytesSHA1B->text();
    QApplication::clipboard()->setText(sha1B);
}

void HexDiffWidget::onCopyShA256BClicked()
{

    const QString sha256B = ui->bytesSHA256B->text();
    QApplication::clipboard()->setText(sha256B);
}

void HexDiffWidget::onCopyCrC32BClicked()
{
    const QString crc32B = ui->bytesCRC32B->text();
    QApplication::clipboard()->setText(crc32B);
}

void HexDiffWidget::clearParseWindow()
{
    ui->bytesEntropyA->setText("");
    ui->bytesMD5A->setText("");
    ui->bytesSHA1A->setText("");
    ui->bytesSHA256A->setText("");
    ui->bytesCRC32A->setText("");

    ui->bytesEntropyB->setText("");
    ui->bytesMD5B->setText("");
    ui->bytesSHA1B->setText("");
    ui->bytesSHA256B->setText("");
    ui->bytesCRC32B->setText("");
}

void HexDiffWidget::updateParseWindow(HexDiffView::Selection selection)
{
    const int size = selection.endAddress - selection.startAddress + 1;
    if (ui->tabParsing->currentIndex() == 1) {
        RzHashSize digestSize = 0;
        const CutterDiffLocked cutterDiff(this->cutterDiff);
        const ut64 oldOffsetA = cutterDiff.coreA->offset;
        const ut64 oldOffsetB = cutterDiff.coreB->offset;
        rz_core_seek(cutterDiff.coreA, selection.startAddress, true);
        rz_core_seek(cutterDiff.coreB, selection.startAddressB, true);
        const ut8 *blockA = cutterDiff.coreA->block;
        const ut8 *blockB = cutterDiff.coreB->block;
        char *digest = rz_hash_cfg_calculate_small_block_string(cutterDiff.coreA->hash, "md5",
                                                                blockA, size, &digestSize, false);
        ui->bytesMD5A->setText(QString(digest));
        free(digest);

        digest = rz_hash_cfg_calculate_small_block_string(cutterDiff.coreB->hash, "md5", blockB,
                                                          size, &digestSize, false);
        ui->bytesMD5B->setText(QString(digest));
        free(digest);

        digest = rz_hash_cfg_calculate_small_block_string(cutterDiff.coreA->hash, "sha1", blockA,
                                                          size, &digestSize, false);
        ui->bytesSHA1A->setText(QString(digest));
        free(digest);

        digest = rz_hash_cfg_calculate_small_block_string(cutterDiff.coreB->hash, "sha1", blockB,
                                                          size, &digestSize, false);
        ui->bytesSHA1B->setText(QString(digest));
        free(digest);

        digest = rz_hash_cfg_calculate_small_block_string(cutterDiff.coreA->hash, "sha256", blockA,
                                                          size, &digestSize, false);
        ui->bytesSHA256A->setText(QString(digest));
        free(digest);

        digest = rz_hash_cfg_calculate_small_block_string(cutterDiff.coreB->hash, "sha256", blockB,
                                                          size, &digestSize, false);
        ui->bytesSHA256B->setText(QString(digest));
        free(digest);

        digest = rz_hash_cfg_calculate_small_block_string(cutterDiff.coreA->hash, "crc32", blockA,
                                                          size, &digestSize, false);
        ui->bytesCRC32A->setText(QString(digest));
        free(digest);

        digest = rz_hash_cfg_calculate_small_block_string(cutterDiff.coreB->hash, "crc32", blockB,
                                                          size, &digestSize, false);
        ui->bytesCRC32B->setText(QString(digest));
        free(digest);

        digest = rz_hash_cfg_calculate_small_block_string(cutterDiff.coreA->hash, "entropy", blockA,
                                                          size, &digestSize, false);
        ui->bytesEntropyA->setText(QString(digest));
        free(digest);

        digest = rz_hash_cfg_calculate_small_block_string(cutterDiff.coreB->hash, "entropy", blockB,
                                                          size, &digestSize, false);
        ui->bytesEntropyB->setText(QString(digest));
        free(digest);

        rz_core_seek(cutterDiff.coreA, oldOffsetA, true);
        rz_core_seek(cutterDiff.coreB, oldOffsetB, true);
        ui->bytesMD5A->setCursorPosition(0);
        ui->bytesSHA1A->setCursorPosition(0);
        ui->bytesSHA256A->setCursorPosition(0);
        ui->bytesCRC32A->setCursorPosition(0);
        ui->bytesMD5B->setCursorPosition(0);
        ui->bytesSHA1B->setCursorPosition(0);
        ui->bytesSHA256B->setCursorPosition(0);
        ui->bytesCRC32B->setCursorPosition(0);
    }
}

void HexDiffWidget::selectionChanged(HexDiffView::Selection selection)
{
    if (selection.empty) {
        clearParseWindow();
    } else {
        updateParseWindow(selection);
    }
}
