#include "HexdumpWidget.h"

#include "common/Configuration.h"
#include "common/Helpers.h"
#include "common/TempConfig.h"
#include "core/MainWindow.h"
#include "ui_HexdumpWidget.h"

#include <QClipboard>
#include <QElapsedTimer>
#include <QInputDialog>
#include <QJsonArray>
#include <QJsonObject>
#include <QMenu>
#include <QScrollBar>
#include <QShortcut>
#include <QTextDocumentFragment>

HexdumpWidget::HexdumpWidget(MainWindow *main)
    : MemoryDockWidget(MemoryWidgetType::Hexdump, main), ui(new Ui::HexdumpWidget)
{
    ui->setupUi(this);

    setObjectName(main ? main->getUniqueObjectName(getWidgetType()) : getWidgetType());
    updateWindowTitle();

    ui->copyMD5->setIcon(QIcon(":/img/icons/copy.svg"));
    ui->copySHA1->setIcon(QIcon(":/img/icons/copy.svg"));
    ui->copySHA256->setIcon(QIcon(":/img/icons/copy.svg"));
    ui->copyCRC32->setIcon(QIcon(":/img/icons/copy.svg"));

    ui->splitter->setChildrenCollapsible(false);

    auto *closeButton = new QToolButton;
    const QIcon closeIcon = QIcon(":/img/icons/delete.svg");
    closeButton->setIcon(closeIcon);
    closeButton->setAutoRaise(true);

    ui->hexSideTab2->setCornerWidget(closeButton);
    syntaxHighLighter = Config()->createSyntaxHighlighter(ui->hexDisasTextEdit->document());

    ui->openSideViewB->hide(); // hide button at startup since side view is visible

    connect(closeButton, &QToolButton::clicked, this, [this] { showSidePanel(false); });

    connect(ui->openSideViewB, &QToolButton::clicked, this, [this] { showSidePanel(true); });

    // Set placeholders for the line-edit components
    const QString placeholder = tr("Select bytes to display information");
    ui->bytesMD5->setPlaceholderText(placeholder);
    ui->bytesEntropy->setPlaceholderText(placeholder);
    ui->bytesSHA1->setPlaceholderText(placeholder);
    ui->bytesSHA256->setPlaceholderText(placeholder);
    ui->bytesCRC32->setPlaceholderText(placeholder);
    ui->hexDisasTextEdit->setPlaceholderText(placeholder);

    setupFonts();

    ui->openSideViewB->setStyleSheet(""
                                     "QToolButton {"
                                     "   border : 0px;"
                                     "   padding : 0px;"
                                     "   margin : 0px;"
                                     "}"
                                     "QToolButton:hover {"
                                     "  border : 1px solid;"
                                     "  border-width : 1px;"
                                     "  border-color : #3daee9"
                                     "}");

    refreshDeferrer = createReplacingRefreshDeferrer<RVA>(
            false, [this](const RVA *offset) { refresh(offset ? *offset : RVA_INVALID); });

    this->ui->hexTextView->addAction(&syncAction);

    connect(Config(), &Configuration::fontsUpdated, this, &HexdumpWidget::fontsUpdated);
    connect(Core(), &CutterCore::refreshAll, this, [this]() { refresh(); });
    connect(Core(), &CutterCore::refreshCodeViews, this, [this]() { refresh(); });
    connect(Core(), &CutterCore::instructionChanged, this, [this]() { refresh(); });
    connect(Core(), &CutterCore::stackChanged, this, [this]() { refresh(); });
    connect(Core(), &CutterCore::registersChanged, this, [this]() { refresh(); });

    connect(seekable, &CutterSeekable::seekableSeekChanged, this, &HexdumpWidget::onSeekChanged);
    connect(ui->hexTextView, &HexWidget::positionChanged, this, [this](RVA addr) {
        if (!sentSeek) {
            sentSeek = true;
            seekable->seek(addr);
            sentSeek = false;
        }
    });
    connect(ui->hexTextView, &HexWidget::selectionChanged, this, &HexdumpWidget::selectionChanged);
    connect(ui->hexSideTab2, &QTabWidget::currentChanged, this,
            &HexdumpWidget::refreshSelectionInfo);
    ui->hexTextView->installEventFilter(this);

    initParsing();
    selectHexPreview();

    connect(ui->parseTypeComboBox, &QComboBox::currentTextChanged, this,
            &HexdumpWidget::onParseTypeComboBoxCurrentTextChanged);
    connect(ui->parseArchComboBox, &QComboBox::currentTextChanged, this,
            &HexdumpWidget::onParseArchComboBoxCurrentTextChanged);
    connect(ui->parseBitsComboBox, &QComboBox::currentTextChanged, this,
            &HexdumpWidget::onParseBitsComboBoxCurrentTextChanged);
    connect(ui->parseEndianComboBox, &QComboBox::currentTextChanged, this,
            &HexdumpWidget::onParseEndianComboBoxCurrentTextChanged);
    connect(ui->hexSideTab2, &QTabWidget::currentChanged, this,
            &HexdumpWidget::onHexSideTab2CurrentChanged);

    connect(ui->copyMD5, &QToolButton::clicked, this, &HexdumpWidget::onCopyMD5Clicked);
    connect(ui->copySHA1, &QToolButton::clicked, this, &HexdumpWidget::onCopyShA1Clicked);
    connect(ui->copySHA256, &QToolButton::clicked, this, &HexdumpWidget::onCopyShA256Clicked);
    connect(ui->copyCRC32, &QToolButton::clicked, this, &HexdumpWidget::onCopyCrC32Clicked);

    // apply initial offset
    refresh(seekable->getOffset());
}

void HexdumpWidget::onSeekChanged(RVA addr)
{
    if (sentSeek) {
        sentSeek = false;
        return;
    }
    refresh(addr);
}

HexdumpWidget::~HexdumpWidget() {}

QString HexdumpWidget::getWidgetType()
{
    return "Hexdump";
}

void HexdumpWidget::refresh()
{
    refresh(RVA_INVALID);
}

void HexdumpWidget::refresh(RVA addr)
{
    if (!refreshDeferrer->attemptRefresh(addr == RVA_INVALID ? nullptr : new RVA(addr))) {
        return;
    }
    sentSeek = true;
    if (addr != RVA_INVALID) {
        ui->hexTextView->seek(addr);
    } else {
        ui->hexTextView->refresh();
        refreshSelectionInfo();
    }
    sentSeek = false;
}

void HexdumpWidget::initParsing()
{
    // Fill the plugins combo for the hexdump sidebar
    ui->parseTypeComboBox->addItem(tr("Disassembly"), "pda");
    ui->parseTypeComboBox->addItem(tr("String"), "pcs");
    ui->parseTypeComboBox->addItem(tr("Assembler"), "pca");
    ui->parseTypeComboBox->addItem(tr("C bytes"), "pc");
    ui->parseTypeComboBox->addItem(tr("C half-words (2 byte)"), "pch");
    ui->parseTypeComboBox->addItem(tr("C words (4 byte)"), "pcw");
    ui->parseTypeComboBox->addItem(tr("C dwords (8 byte)"), "pcd");
    ui->parseTypeComboBox->addItem(tr("Python"), "pcp");
    ui->parseTypeComboBox->addItem(tr("JSON"), "pcj");
    ui->parseTypeComboBox->addItem(tr("JavaScript"), "pcJ");
    ui->parseTypeComboBox->addItem(tr("Yara"), "pcy");

    ui->parseArchComboBox->insertItems(0, Core()->getAsmPluginNames());

    ui->parseEndianComboBox->setCurrentIndex(Core()->getConfigb("cfg.bigendian") ? 1 : 0);
}

void HexdumpWidget::selectionChanged(HexWidget::Selection selection)
{
    if (selection.empty) {
        clearParseWindow();
    } else {
        updateParseWindow(selection.startAddress,
                          selection.endAddress - selection.startAddress + 1);
    }
}

void HexdumpWidget::onParseArchComboBoxCurrentTextChanged(const QString & /*arg1*/)
{
    refreshSelectionInfo();
}

void HexdumpWidget::onParseBitsComboBoxCurrentTextChanged(const QString & /*arg1*/)
{
    refreshSelectionInfo();
}

void HexdumpWidget::setupFonts()
{
    const QFont font = Config()->getFont();
    ui->hexDisasTextEdit->setFont(font);
    ui->hexTextView->setMonospaceFont(font);
}

void HexdumpWidget::refreshSelectionInfo()
{
    selectionChanged(ui->hexTextView->getSelection());
}

void HexdumpWidget::fontsUpdated()
{
    setupFonts();
}

void HexdumpWidget::clearParseWindow()
{
    ui->hexDisasTextEdit->setPlainText("");
    ui->bytesEntropy->setText("");
    ui->bytesMD5->setText("");
    ui->bytesSHA1->setText("");
    ui->bytesSHA256->setText("");
    ui->bytesCRC32->setText("");
}

void HexdumpWidget::showSidePanel(bool show)
{
    ui->hexSideTab2->setVisible(show);
    ui->openSideViewB->setHidden(show);
    if (show) {
        refreshSelectionInfo();
    }
}

QString HexdumpWidget::getWindowTitle() const
{
    return tr("Hexdump");
}

void HexdumpWidget::updateParseWindow(RVA start_address, int size)
{
    if (!ui->hexSideTab2->isVisible()) {
        return;
    }

    if (ui->hexSideTab2->currentIndex() == 0) {
        // scope for TempConfig

        // Get selected combos
        const QString arch = ui->parseArchComboBox->currentText();
        const QString bits = ui->parseBitsComboBox->currentText();
        const QString selectedCommand = ui->parseTypeComboBox->currentData().toString();
        const QString commandResult = "";
        const bool bigEndian = ui->parseEndianComboBox->currentIndex() == 1;

        TempConfig tempConfig;
        tempConfig.set("asm.arch", arch).set("asm.bits", bits).set("cfg.bigendian", bigEndian);

        ui->hexDisasTextEdit->setPlainText(
                selectedCommand != ""
                        ? Core()->cmdRawAt(QString("%1 @! %2").arg(selectedCommand).arg(size),
                                           start_address)
                        : "");
    } else {
        // Fill the information tab hashes and entropy
        RzHashSize digestSize = 0;
        RzCoreLocked core(Core());
        const ut64 oldOffset = core->offset;
        rz_core_seek(core, start_address, true);
        const ut8 *block = core->block;
        char *digest = rz_hash_cfg_calculate_small_block_string(core->hash, "md5", block, size,
                                                                &digestSize, false);
        ui->bytesMD5->setText(QString(digest));
        free(digest);
        digest = rz_hash_cfg_calculate_small_block_string(core->hash, "sha1", block, size,
                                                          &digestSize, false);
        ui->bytesSHA1->setText(QString(digest));
        free(digest);
        digest = rz_hash_cfg_calculate_small_block_string(core->hash, "sha256", block, size,
                                                          &digestSize, false);
        ui->bytesSHA256->setText(QString(digest));
        free(digest);
        digest = rz_hash_cfg_calculate_small_block_string(core->hash, "crc32", block, size,
                                                          &digestSize, false);
        ui->bytesCRC32->setText(QString(digest));
        free(digest);
        digest = rz_hash_cfg_calculate_small_block_string(core->hash, "entropy", block, size,
                                                          &digestSize, false);
        ui->bytesEntropy->setText(QString(digest));
        free(digest);
        rz_core_seek(core, oldOffset, true);
        ui->bytesMD5->setCursorPosition(0);
        ui->bytesSHA1->setCursorPosition(0);
        ui->bytesSHA256->setCursorPosition(0);
        ui->bytesCRC32->setCursorPosition(0);
    }
}

void HexdumpWidget::onParseTypeComboBoxCurrentTextChanged(const QString &)
{
    const QString currentParseTypeText = ui->parseTypeComboBox->currentData().toString();
    if (currentParseTypeText == "pda" || currentParseTypeText == "pci") {
        ui->hexSideFrame2->show();
    } else {
        ui->hexSideFrame2->hide();
    }
    refreshSelectionInfo();
}

void HexdumpWidget::onParseEndianComboBoxCurrentTextChanged(const QString &)
{
    refreshSelectionInfo();
}

void HexdumpWidget::onHexSideTab2CurrentChanged(int /*index*/)
{
    /*
    if (index == 2) {
        // Add data to HTML Polar functions graph
        QFile html(":/html/bar.html");
        if(!html.open(QIODevice::ReadOnly)) {
            QMessageBox::information(0,"error",html.errorString());
        }
        QString code = html.readAll();
        html.close();
        this->histoWebView->setHtml(code);
        this->histoWebView->show();
    } else {
        this->histoWebView->hide();
    }
    */
}

void HexdumpWidget::resizeEvent(QResizeEvent *event)
{
    // Heuristics to hide sidebar when it hides the content of the hexdump. 600px looks just "okay"
    // Only applied when widget width is decreased to avoid unwanted behavior
    if (event->oldSize().width() > event->size().width() && event->size().width() < 600) {
        showSidePanel(false);
    }
    QDockWidget::resizeEvent(event);
    refresh();
}

QWidget *HexdumpWidget::widgetToFocusOnRaise()
{
    return ui->hexTextView;
}

void HexdumpWidget::onCopyMD5Clicked()
{
    const QString md5 = ui->bytesMD5->text();
    QClipboard *clipboard = QApplication::clipboard();
    clipboard->setText(md5);
    Core()->message("MD5 copied to clipboard: " + md5);
}

void HexdumpWidget::onCopyShA1Clicked()
{
    const QString sha1 = ui->bytesSHA1->text();
    QClipboard *clipboard = QApplication::clipboard();
    clipboard->setText(sha1);
    Core()->message("SHA1 copied to clipboard: " + sha1);
}

void HexdumpWidget::onCopyShA256Clicked()
{
    const QString sha256 = ui->bytesSHA256->text();
    QClipboard *clipboard = QApplication::clipboard();
    clipboard->setText(sha256);
    Core()->message("SHA256 copied to clipboard: " + sha256);
}

void HexdumpWidget::onCopyCrC32Clicked()
{
    const QString crc32 = ui->bytesCRC32->text();
    QClipboard *clipboard = QApplication::clipboard();
    clipboard->setText(crc32);
    Core()->message("CRC32 copied to clipboard: " + crc32);
}

void HexdumpWidget::selectHexPreview()
{
    // Pre-select arch and bits in the hexdump sidebar
    const QString arch = Core()->getConfig("asm.arch");
    const QString bits = Core()->getConfig("asm.bits");

    if (ui->parseArchComboBox->findText(arch) != -1) {
        ui->parseArchComboBox->setCurrentIndex(ui->parseArchComboBox->findText(arch));
    }

    if (ui->parseBitsComboBox->findText(bits) != -1) {
        ui->parseBitsComboBox->setCurrentIndex(ui->parseBitsComboBox->findText(bits));
    }
}
