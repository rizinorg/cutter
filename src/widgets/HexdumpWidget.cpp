#include "HexdumpWidget.h"
#include "ui_HexdumpWidget.h"

#include "common/Helpers.h"
#include "common/Configuration.h"
#include "common/TempConfig.h"

#include <QJsonObject>
#include <QJsonArray>
#include <QElapsedTimer>
#include <QTextDocumentFragment>
#include <QMenu>
#include <QClipboard>
#include <QScrollBar>
#include <QInputDialog>
#include <QShortcut>

HexdumpWidget::HexdumpWidget(MainWindow *main, QAction *action) :
    MemoryDockWidget(CutterCore::MemoryWidgetType::Hexdump, main, action),
    ui(new Ui::HexdumpWidget),
    seekable(new CutterSeekable(this))
{
    ui->setupUi(this);

    /*
     * Ugly hack just for the layout issue
     * QSettings saves the state with the object names
     * By doing this hack,
     * you can at least avoid some mess by dismissing all the Extra Widgets
     */
    QString name = "Hexdump";
    if (!action) {
        name = "Extra Hexdump";
    }
    setObjectName(name);

    ui->copyMD5->setIcon(QIcon(":/img/icons/copy.svg"));
    ui->copySHA1->setIcon(QIcon(":/img/icons/copy.svg"));


    ui->splitter->setChildrenCollapsible(false);

    QToolButton *closeButton = new QToolButton;
    QIcon closeIcon = QIcon(":/img/icons/delete.svg");
    closeButton->setIcon(closeIcon);
    closeButton->setAutoRaise(true);
    ui->hexSideTab_2->setCornerWidget(closeButton);

    ui->openSideViewB->hide();  // hide button at startup since side view is visible

    connect(closeButton, &QToolButton::clicked, this, [this] {
        ui->hexSideTab_2->hide();
        ui->openSideViewB->show();
    });

    connect(ui->openSideViewB, &QToolButton::clicked, this, [this] {
        ui->hexSideTab_2->show();
        ui->openSideViewB->hide();
    });

    ui->bytesMD5->setPlaceholderText("Select bytes to display information");
    ui->bytesEntropy->setPlaceholderText("Select bytes to display information");
    ui->bytesSHA1->setPlaceholderText("Select bytes to display information");
    ui->hexDisasTextEdit->setPlaceholderText("Select bytes to display information");

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

    this->setWindowTitle(tr("Hexdump"));

    refreshDeferrer = createReplacingRefreshDeferrer<RVA>(false, [this](const RVA *offset) {
        refresh(offset ? *offset : RVA_INVALID);
    });

    connect(&syncAction, SIGNAL(triggered(bool)), this, SLOT(toggleSync()));
    syncAction.setText(tr("Sync/unsync offset"));
    this->ui->hexTextView->addAction(&syncAction);

    connect(Config(), SIGNAL(fontsUpdated()), this, SLOT(fontsUpdated()));

    connect(this, &QDockWidget::visibilityChanged, this, [](bool visibility) {
        if (visibility) {
            Core()->setMemoryWidgetPriority(CutterCore::MemoryWidgetType::Hexdump);
        }
    });

    connect(Core(), &CutterCore::refreshAll, this, [this]() { refresh(); });

    connect(seekable, &CutterSeekable::seekableSeekChanged, this, &HexdumpWidget::onSeekChanged);
    connect(ui->hexTextView, &HexWidget::positionChanged, this, [this](RVA addr) {
        if (!sent_seek) {
            sent_seek = true;
            seekable->seek(addr);
            sent_seek = false;
        }
    });
    connect(ui->hexTextView, &HexWidget::selectionChanged, this, &HexdumpWidget::selectionChanged);

    initParsing();
    selectHexPreview();
}

void HexdumpWidget::onSeekChanged(RVA addr)
{
    if (sent_seek) {
        sent_seek = false;
        return;
    }
    refresh(addr);
}

HexdumpWidget::~HexdumpWidget() {}

void HexdumpWidget::refresh()
{
    refresh(RVA_INVALID);
}

void HexdumpWidget::refresh(RVA addr)
{
    if (!refreshDeferrer->attemptRefresh(addr == RVA_INVALID ? nullptr : new RVA(addr))) {
        return;
    }
    sent_seek = true;
    if (addr != RVA_INVALID) {
        ui->hexTextView->seek(addr);
    } else {
        ui->hexTextView->refresh();
        refreshSelectionInfo();
    }
    sent_seek = false;
}


void HexdumpWidget::initParsing()
{
    // Fill the plugins combo for the hexdump sidebar
    ui->parseArchComboBox->insertItems(0, Core()->getAsmPluginNames());

    ui->parseEndianComboBox->setCurrentIndex(Core()->getConfigb("cfg.bigendian") ? 1 : 0);
}

void HexdumpWidget::selectionChanged(HexWidget::Selection selection)
{
    if (selection.empty) {
        clearParseWindow();
    } else {
        updateParseWindow(selection.startAddress, selection.endAddress - selection.startAddress + 1);
    }
}

void HexdumpWidget::on_parseArchComboBox_currentTextChanged(const QString &/*arg1*/)
{
    refreshSelectionInfo();
}

void HexdumpWidget::on_parseBitsComboBox_currentTextChanged(const QString &/*arg1*/)
{
    refreshSelectionInfo();
}


void HexdumpWidget::toggleSync()
{
    QString windowTitle = tr("Hexdump");
    seekable->toggleSynchronization();
    if (seekable->isSynchronized()) {
        setWindowTitle(windowTitle);
    } else {
        setWindowTitle(windowTitle + CutterSeekable::tr(" (unsynced)"));
    }
}


void HexdumpWidget::setupFonts()
{
    QFont font = Config()->getFont();
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
}

void HexdumpWidget::updateParseWindow(RVA start_address, int size)
{

    QString address = RAddressString(start_address);

    QString argument = QString("%1@" + address).arg(size);
    // Get selected combos
    QString arch = ui->parseArchComboBox->currentText();
    QString bits = ui->parseBitsComboBox->currentText();
    bool bigEndian = ui->parseEndianComboBox->currentIndex() == 1;

    {
        // scope for TempConfig
        TempConfig tempConfig;
        tempConfig
        .set("asm.arch", arch)
        .set("asm.bits", bits)
        .set("cfg.bigendian", bigEndian);

        switch (ui->parseTypeComboBox->currentIndex()) {
        case 0: // Disassembly
            ui->hexDisasTextEdit->setPlainText(Core()->cmd("pda " + argument));
            break;
        case 1: // String
            ui->hexDisasTextEdit->setPlainText(Core()->cmd("pcs " + argument));
            break;
        case 2: // Assembler
            ui->hexDisasTextEdit->setPlainText(Core()->cmd("pca " + argument));
            break;
        case 3: // C byte array
            ui->hexDisasTextEdit->setPlainText(Core()->cmd("pc " + argument));
            break;
        case 4: // C half-word
            ui->hexDisasTextEdit->setPlainText(Core()->cmd("pch " + argument));
            break;
        case 5: // C word
            ui->hexDisasTextEdit->setPlainText(Core()->cmd("pcw " + argument));
            break;
        case 6: // C dword
            ui->hexDisasTextEdit->setPlainText(Core()->cmd("pcd " + argument));
            break;
        case 7: // Python
            ui->hexDisasTextEdit->setPlainText(Core()->cmd("pcp " + argument));
            break;
        case 8: // JSON
            ui->hexDisasTextEdit->setPlainText(Core()->cmd("pcj " + argument));
            break;
        case 9: // JavaScript
            ui->hexDisasTextEdit->setPlainText(Core()->cmd("pcJ " + argument));
            break;
        default:
            ui->hexDisasTextEdit->setPlainText("");
        }
    }

    // Fill the information tab hashes and entropy
    ui->bytesMD5->setText(Core()->cmd("ph md5 " + argument).trimmed());
    ui->bytesSHA1->setText(Core()->cmd("ph sha1 " + argument).trimmed());
    ui->bytesEntropy->setText(Core()->cmd("ph entropy " + argument).trimmed());
    ui->bytesMD5->setCursorPosition(0);
    ui->bytesSHA1->setCursorPosition(0);
}

/*
 * Actions callback functions
 */

void HexdumpWidget::on_actionHideHexdump_side_panel_triggered()
{
    if (ui->hexSideTab_2->isVisible()) {
        ui->hexSideTab_2->hide();
    } else {
        ui->hexSideTab_2->show();
    }
}


void HexdumpWidget::on_parseTypeComboBox_currentTextChanged(const QString &)
{
    if (ui->parseTypeComboBox->currentIndex() == 0) {
        ui->hexSideFrame_2->show();
    } else {
        ui->hexSideFrame_2->hide();
    }
    refreshSelectionInfo();
}

void HexdumpWidget::on_parseEndianComboBox_currentTextChanged(const QString &)
{
    refreshSelectionInfo();
}

void HexdumpWidget::on_hexSideTab_2_currentChanged(int /*index*/)
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
    QDockWidget::resizeEvent(event);
    refresh();
}

QWidget *HexdumpWidget::widgetToFocusOnRaise()
{
    return ui->hexTextView;
}

void HexdumpWidget::on_copyMD5_clicked()
{
    QString md5 = ui->bytesMD5->text();
    QClipboard *clipboard = QApplication::clipboard();
    clipboard->setText(md5);
    // FIXME
    // this->main->addOutput("MD5 copied to clipboard: " + md5);
}

void HexdumpWidget::on_copySHA1_clicked()
{
    QString sha1 = ui->bytesSHA1->text();
    QClipboard *clipboard = QApplication::clipboard();
    clipboard->setText(sha1);
    // FIXME
    // this->main->addOutput("SHA1 copied to clipboard: " + sha1);
}


void HexdumpWidget::selectHexPreview()
{
    // Pre-select arch and bits in the hexdump sidebar
    QString arch = Core()->getConfig("asm.arch");
    QString bits = Core()->getConfig("asm.bits");

    if (ui->parseArchComboBox->findText(arch) != -1) {
        ui->parseArchComboBox->setCurrentIndex(ui->parseArchComboBox->findText(arch));
    }

    if (ui->parseBitsComboBox->findText(bits) != -1) {
        ui->parseBitsComboBox->setCurrentIndex(ui->parseBitsComboBox->findText(bits));
    }
}
