#include "HexdumpWidget.h"
#include "ui_HexdumpWidget.h"

#include "utils/Helpers.h"
#include "utils/Configuration.h"
#include "utils/TempConfig.h"

#include <QJsonObject>
#include <QJsonArray>
#include <QElapsedTimer>
#include <QTextDocumentFragment>
#include <QMenu>
#include <QClipboard>
#include <QScrollBar>

HexdumpWidget::HexdumpWidget(MainWindow *main, QAction *action) :
    CutterDockWidget(main, action),
    ui(new Ui::HexdumpWidget),
    seekable(new CutterSeekableWidget(this))
{
    ui->setupUi(this);

    // Setup hex highlight
    //connect(ui->hexHexText, SIGNAL(cursorPositionChanged()), this, SLOT(highlightHexCurrentLine()));
    //highlightHexCurrentLine();

    ui->copyMD5->setIcon(QIcon(new SvgIconEngine(QString(":/img/icons/transfer.svg"),
                                                 palette().buttonText().color())));
    ui->copySHA1->setIcon(QIcon(new SvgIconEngine(QString(":/img/icons/transfer.svg"),
                                                  palette().buttonText().color())));

    int margin = static_cast<int>(ui->hexOffsetText->document()->documentMargin());
    ui->offsetHeaderLabel->setContentsMargins(margin, 0, margin, 0);

    margin = static_cast<int>(ui->hexHexText->document()->documentMargin());
    ui->hexHeaderLabel->setContentsMargins(margin, 0, margin, 0);

    margin = static_cast<int>(ui->hexASCIIText->document()->documentMargin());
    ui->asciiHeaderLabel->setContentsMargins(margin, 0, margin, 0);

    ui->splitter->setCollapsible(0, false); // Only Sidebar should collapse

    setupFonts();

    colorsUpdatedSlot();
    updateHeaders();

    this->setWindowTitle(tr("Hexdump"));
    connect(&syncAction, SIGNAL(triggered(bool)), this, SLOT(toggleSync()));

    // Set hexdump context menu
    ui->hexHexText->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(ui->hexHexText, SIGNAL(customContextMenuRequested(const QPoint &)),
            this, SLOT(showHexdumpContextMenu(const QPoint &)));
    ui->hexASCIIText->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(ui->hexASCIIText, SIGNAL(customContextMenuRequested(const QPoint &)),
            this, SLOT(showHexASCIIContextMenu(const QPoint &)));

    setupScrollSync();

    // Control Disasm and Hex scroll to add more contents
    connectScroll(false);

    connect(Config(), SIGNAL(fontsUpdated()), this, SLOT(fontsUpdated()));
    connect(Config(), SIGNAL(colorsUpdated()), this, SLOT(colorsUpdatedSlot()));

    connect(Core(), SIGNAL(raisePrioritizedMemoryWidget(CutterCore::MemoryWidgetType)), this,
            SLOT(raisePrioritizedMemoryWidget(CutterCore::MemoryWidgetType)));

    connect(this, &QDockWidget::visibilityChanged, this, [](bool visibility) {
        if (visibility) {
            Core()->setMemoryWidgetPriority(CutterCore::MemoryWidgetType::Hexdump);
        }
    });

    connect(Core(), &CutterCore::refreshAll, this, [this]() {
        refresh(seekable->getOffset());
    });

    connect(ui->hexHexText, &QTextEdit::selectionChanged, this, &HexdumpWidget::selectionChanged);
    connect(ui->hexASCIIText, &QTextEdit::selectionChanged, this, &HexdumpWidget::selectionChanged);
    connect(ui->hexHexText, &QTextEdit::cursorPositionChanged, this, &HexdumpWidget::selectionChanged);
    connect(ui->hexASCIIText, &QTextEdit::cursorPositionChanged, this,
            &HexdumpWidget::selectionChanged);
    connect(seekable, &CutterSeekableWidget::seekChanged, this, &HexdumpWidget::on_seekChanged);

    format = Format::Hex;
    initParsing();
    selectHexPreview();
}

void HexdumpWidget::setupScrollSync()
{
    /*
     * For some reason, QScrollBar::valueChanged is not emitted when
     * the scrolling happened from moving the cursor beyond the visible content,
     * so QTextEdit::cursorPositionChanged has to be connected as well.
     */

    auto offsetHexFunc = [this]() {
        if (!scroll_disabled) {
            scroll_disabled = true;
            ui->hexHexText->verticalScrollBar()->setValue(ui->hexOffsetText->verticalScrollBar()->value());
            scroll_disabled = false;
        }
    };

    auto offsetASCIIFunc = [this]() {
        if (!scroll_disabled) {
            scroll_disabled = true;
            ui->hexASCIIText->verticalScrollBar()->setValue(ui->hexOffsetText->verticalScrollBar()->value());
            scroll_disabled = false;
        }
    };

    connect(ui->hexOffsetText->verticalScrollBar(), &QScrollBar::valueChanged,
            ui->hexHexText->verticalScrollBar(), offsetHexFunc);
    connect(ui->hexOffsetText, &QTextEdit::cursorPositionChanged, ui->hexHexText->verticalScrollBar(),
            offsetHexFunc);
    connect(ui->hexOffsetText->verticalScrollBar(), &QScrollBar::valueChanged,
            ui->hexASCIIText->verticalScrollBar(), offsetASCIIFunc);
    connect(ui->hexOffsetText, &QTextEdit::cursorPositionChanged, ui->hexASCIIText->verticalScrollBar(),
            offsetASCIIFunc);

    auto hexOffsetFunc = [this]() {
        if (!scroll_disabled) {
            scroll_disabled = true;
            ui->hexOffsetText->verticalScrollBar()->setValue(ui->hexHexText->verticalScrollBar()->value());
            scroll_disabled = false;
        }
    };

    auto hexASCIIFunc = [this]() {
        if (!scroll_disabled) {
            scroll_disabled = true;
            ui->hexASCIIText->verticalScrollBar()->setValue(ui->hexHexText->verticalScrollBar()->value());
            scroll_disabled = false;
        }
    };

    connect(ui->hexHexText->verticalScrollBar(), &QScrollBar::valueChanged,
            ui->hexOffsetText->verticalScrollBar(), hexOffsetFunc);
    connect(ui->hexHexText, &QTextEdit::cursorPositionChanged, ui->hexOffsetText->verticalScrollBar(),
            hexOffsetFunc);
    connect(ui->hexHexText->verticalScrollBar(), &QScrollBar::valueChanged,
            ui->hexASCIIText->verticalScrollBar(), hexASCIIFunc);
    connect(ui->hexHexText, &QTextEdit::cursorPositionChanged, ui->hexASCIIText->verticalScrollBar(),
            hexASCIIFunc);

    auto asciiOffsetFunc = [this]() {
        if (!scroll_disabled) {
            scroll_disabled = true;
            ui->hexOffsetText->verticalScrollBar()->setValue(ui->hexASCIIText->verticalScrollBar()->value());
            scroll_disabled = false;
        }
    };

    auto asciiHexFunc = [this]() {
        if (!scroll_disabled) {
            scroll_disabled = true;
            ui->hexHexText->verticalScrollBar()->setValue(ui->hexASCIIText->verticalScrollBar()->value());
            scroll_disabled = false;
        }
    };

    connect(ui->hexASCIIText->verticalScrollBar(), &QScrollBar::valueChanged,
            ui->hexOffsetText->verticalScrollBar(), asciiOffsetFunc);
    connect(ui->hexASCIIText, &QTextEdit::cursorPositionChanged, ui->hexOffsetText->verticalScrollBar(),
            asciiOffsetFunc);
    connect(ui->hexASCIIText->verticalScrollBar(), &QScrollBar::valueChanged,
            ui->hexHexText->verticalScrollBar(), asciiHexFunc);
    connect(ui->hexASCIIText, &QTextEdit::cursorPositionChanged, ui->hexHexText->verticalScrollBar(),
            asciiHexFunc);
}

void HexdumpWidget::on_seekChanged(RVA addr)
{
    if (sent_seek) {
        sent_seek = false;
        return;
    }
    refresh(addr);
}

void HexdumpWidget::raisePrioritizedMemoryWidget(CutterCore::MemoryWidgetType type)
{
    if (type == CutterCore::MemoryWidgetType::Hexdump) {
        raise();
    }
}

void HexdumpWidget::connectScroll(bool disconnect_)
{
    scroll_disabled = disconnect_;
    if (disconnect_) {
        disconnect(ui->hexHexText->verticalScrollBar(), &QScrollBar::valueChanged, this,
                   &HexdumpWidget::scrollChanged);
        disconnect(ui->hexHexText, &QTextEdit::cursorPositionChanged, this, &HexdumpWidget::scrollChanged);
    } else {
        connect(ui->hexHexText->verticalScrollBar(), &QScrollBar::valueChanged, this,
                &HexdumpWidget::scrollChanged);
        connect(ui->hexHexText, &QTextEdit::cursorPositionChanged, this, &HexdumpWidget::scrollChanged);

    }
}

HexdumpWidget::~HexdumpWidget() {}

/*
 * Text highlight functions
 * Currently unused
 */
/*
void HexdumpWidget::highlightHexCurrentLine()
{
    QList<QTextEdit::ExtraSelection> extraSelections;

    if (!ui->hexHexText->isReadOnly())
    {
        QTextEdit::ExtraSelection selection;

        QColor lineColor = QColor(190, 144, 212);

        selection.format.setBackground(lineColor);
        selection.format.setProperty(QTextFormat::FullWidthSelection, true);
        selection.cursor = ui->hexHexText->textCursor();
        selection.cursor.clearSelection();
        extraSelections.append(selection);
    }

    QTextCursor cursor = ui->hexHexText->textCursor();
    cursor.select(QTextCursor::WordUnderCursor);

    QTextEdit::ExtraSelection currentWord;

    QColor blueColor = QColor(Qt::blue).lighter(160);
    currentWord.format.setBackground(blueColor);

    currentWord.cursor = cursor;
    extraSelections.append(currentWord);

    ui->hexHexText->setExtraSelections(extraSelections);

    highlightHexWords(cursor.selectedText());
}


void HexdumpWidget::highlightHexWords(const QString &str)
{
    QString searchString = str;
    QTextDocument *document = ui->hexHexText->document();

    document->undo();

    QTextCursor highlightCursor(document);
    QTextCursor cursor(document);

    cursor.beginEditBlock();

    QColor blueColor = QColor(Qt::blue).lighter(160);

    QTextCharFormat plainFormat(highlightCursor.charFormat());
    QTextCharFormat colorFormat = plainFormat;
    colorFormat.setBackground(blueColor);

    while (!highlightCursor.isNull() && !highlightCursor.atEnd())
    {
        highlightCursor = document->find(searchString, highlightCursor, QTextDocument::FindWholeWords);

        if (!highlightCursor.isNull())
        {
            highlightCursor.movePosition(QTextCursor::EndOfWord, QTextCursor::KeepAnchor);
            highlightCursor.mergeCharFormat(colorFormat);
        }
    }
    cursor.endEditBlock();
}
*/

void HexdumpWidget::refresh(RVA addr)
{
    connectScroll(true);

    updateHeaders();

    if (addr == RVA_INVALID) {
        addr = seekable->getOffset();
    }

    cols = Core()->getConfigi("hex.cols");
    // Avoid divison by 0
    if (cols == 0)
        cols = 16;

    // TODO: Figure out how to calculate a sane value for this
    bufferLines = qhelpers::getMaxFullyDisplayedLines(ui->hexHexText);

    ut64 loadLines = bufferLines * 3; // total lines to load
    ut64 curAddrLineOffset = bufferLines; // line number where seek should be

    if (addr < curAddrLineOffset * cols) {
        curAddrLineOffset = static_cast<int>(addr / cols);
    }

    if (addr > RVA_MAX - curAddrLineOffset * cols) {
        curAddrLineOffset = static_cast<int>(loadLines - (RVA_MAX - addr) / cols);
    }

    first_loaded_address = addr - curAddrLineOffset * cols;
    last_loaded_address = addr + (loadLines - curAddrLineOffset) * cols;

    auto hexdump = fetchHexdump(first_loaded_address, loadLines);

    ui->hexOffsetText->setText(hexdump[0]);
    ui->hexHexText->setText(hexdump[1]);
    ui->hexASCIIText->setPlainText(hexdump[2]);

    QTextCursor cursor(ui->hexHexText->document()->findBlockByLineNumber(curAddrLineOffset));
    ui->hexHexText->moveCursor(QTextCursor::End);
    ui->hexHexText->ensureCursorVisible();
    ui->hexHexText->setTextCursor(cursor);
    ui->hexHexText->ensureCursorVisible();

    // Set the backgorund color of the current seek
    QTextCursor offsetCursor(ui->hexOffsetText->document()->findBlockByLineNumber(curAddrLineOffset));
    QTextBlockFormat formatTmp = offsetCursor.blockFormat();
    formatTmp.setBackground(QColor(64, 129, 160));
    offsetCursor.setBlockFormat(formatTmp);

    updateWidths();

    // Update other text areas scroll
    ui->hexOffsetText->verticalScrollBar()->setValue(ui->hexHexText->verticalScrollBar()->value());
    ui->hexASCIIText->verticalScrollBar()->setValue(ui->hexHexText->verticalScrollBar()->value());

    connectScroll(false);
}

void HexdumpWidget::updateHeaders()
{
    int cols = Core()->getConfigi("hex.cols");
    int ascii_cols = cols;
    bool pairs = Core()->getConfigb("hex.pairs");

    QString hexHeaderString;
    QString asciiHeaderString;

    QTextStream hexHeader(&hexHeaderString);
    QTextStream asciiHeader(&asciiHeaderString);

    hexHeader.setIntegerBase(16);
    hexHeader.setNumberFlags(QTextStream::UppercaseDigits);
    asciiHeader.setIntegerBase(16);
    asciiHeader.setNumberFlags(QTextStream::UppercaseDigits);

    // Custom spacing for the header
    QString space = " ";
    switch (format) {
    case Hex:
        space = space.repeated(1);
        break;
    case Octal:
        space = space.repeated(2);
        break;
    default:
        qWarning() << "Unknown format in hexdump!";
        break;
    }

    for (int i = 0; i < cols; i++) {
        if (i > 0 && ((pairs && !(i & 1)) || !pairs)) {
            hexHeader << " ";
        }

        hexHeader << space << (i & 0xF);
    }

    for (int i = 0; i < ascii_cols; i++) {
        asciiHeader << (i & 0xF);
    }

    hexHeader.flush();
    asciiHeader.flush();

    ui->hexHeaderLabel->setText(hexHeaderString);
    ui->asciiHeaderLabel->setText(asciiHeaderString);
}

void HexdumpWidget::initParsing()
{
    // Fill the plugins combo for the hexdump sidebar
    ui->parseArchComboBox->insertItems(0, Core()->getAsmPluginNames());

    ui->parseEndianComboBox->setCurrentIndex(Core()->getConfigb("cfg.bigendian") ? 1 : 0);
}

std::array<QString, 3> HexdumpWidget::fetchHexdump(RVA addr, int lines)
{
    // Main bytes to fetch:
    int bytes = cols * lines;

    QString command = QString("pxj %1 @%2").arg(
                          QString::number(bytes),
                          RAddressString(addr));
    QJsonArray byte_array = Core()->cmdj(command).array();

    QString hexText = "";
    QString offsetText = "";
    QString asciiText = "";
    RVA cur_addr = addr;
    for (int i = 0; i < lines; i++) {
        for (int j = 0; j < cols; j++) {
            int b = byte_array[(i * cols) + j].toInt();
            if ((j > 0) && (j < cols)) {
                hexText += " ";
            }
            // Non printable
            if ((b < 0x20) || (b > 0x7E)) {
                asciiText += ".";
            } else {
                asciiText += (char)b;
            }

            switch (format) {
            case Octal:
                hexText += QString::number(b, 8).rightJustified(3, '0');
                break;
            case Hex:
            default:
                hexText += QString::number(b, 16).rightJustified(2, '0');
                break;
            }
        }
        offsetText += RAddressString(cur_addr) + "\n";
        hexText += "\n";
        asciiText += "\n";
        cur_addr += cols;
    }

    return {{offsetText, hexText, asciiText}};
}

void HexdumpWidget::selectionChanged()
{
    if (scroll_disabled) {
        return;
    }
    connectScroll(true);

    if (sender() == ui->hexHexText) {
        QTextCursor textCursor = ui->hexHexText->textCursor();
        if (!textCursor.hasSelection()) {
            clearParseWindow();
            RVA adr = hexPositionToAddress(textCursor.position());
            int pos = asciiAddressToPosition(adr);
            setTextEditPosition(ui->hexASCIIText, pos);
            sent_seek = true;
            seekable->seek(adr);
            sent_seek = false;
            connectScroll(false);
            return;
        }

        int selectionStart = textCursor.selectionStart();
        int selectionEnd = textCursor.selectionEnd();

        QChar start = ui->hexHexText->document()->characterAt(selectionStart);
        QChar end = ui->hexHexText->document()->characterAt(selectionEnd);

        // This adjusts the selection to make sense with the chosen format
        switch (format) {
        case Hex:
            // Handle the spaces/newlines (if it's at the start, move forward,
            // if it's at the end, move back)

            if (!start.isLetterOrNumber()) {
                selectionStart += 1;
            } else if (ui->hexHexText->document()->characterAt(selectionStart - 1).isLetterOrNumber()) {
                selectionStart += 2;
            }

            if (!end.isLetterOrNumber()) {
                selectionEnd += 1;
            }
            break;
        case Octal:
            if (!start.isLetterOrNumber()) {
                selectionStart += 1;
            }
            if (!end.isLetterOrNumber()) {
                selectionEnd += 1;
            }
            break;
        }

        // In hextext we have the spaces that we need to somehow handle.
        RVA startAddress = hexPositionToAddress(selectionStart);
        RVA endAddress = hexPositionToAddress(selectionEnd);

        updateParseWindow(startAddress, endAddress - startAddress);

        int startPosition = asciiAddressToPosition(startAddress);
        int endPosition = asciiAddressToPosition(endAddress);
        QTextCursor targetTextCursor = ui->hexASCIIText->textCursor();
        targetTextCursor.setPosition(startPosition);
        targetTextCursor.setPosition(endPosition, QTextCursor::KeepAnchor);
        ui->hexASCIIText->setTextCursor(targetTextCursor);
        sent_seek = true;
        seekable->seek(startAddress);
        sent_seek = false;
    } else {
        QTextCursor textCursor = ui->hexASCIIText->textCursor();
        if (!textCursor.hasSelection()) {
            clearParseWindow();
            RVA adr = asciiPositionToAddress(textCursor.position());
            int pos = hexAddressToPosition(adr);
            setTextEditPosition(ui->hexHexText, pos);
            connectScroll(false);
            sent_seek = true;
            seekable->seek(adr);
            sent_seek = false;
            return;
        }
        RVA startAddress = asciiPositionToAddress(textCursor.selectionStart());
        RVA endAddress = asciiPositionToAddress(textCursor.selectionEnd());

        updateParseWindow(startAddress, endAddress - startAddress);

        int startPosition = hexAddressToPosition(startAddress);
        int endPosition = hexAddressToPosition(endAddress);

        // End position -1 because the position we get above is for the next
        // entry, so including the space/newline
        endPosition -= 1;
        QTextCursor targetTextCursor = ui->hexHexText->textCursor();
        targetTextCursor.setPosition(startPosition);
        targetTextCursor.setPosition(endPosition, QTextCursor::KeepAnchor);
        ui->hexHexText->setTextCursor(targetTextCursor);
        sent_seek = true;
        seekable->seek(startAddress);
        sent_seek = false;
    }

    connectScroll(false);
    return;
}

void HexdumpWidget::on_parseArchComboBox_currentTextChanged(const QString &/*arg1*/)
{
    selectionChanged();
}

void HexdumpWidget::on_parseBitsComboBox_currentTextChanged(const QString &/*arg1*/)
{
    selectionChanged();
}

void HexdumpWidget::showHexdumpContextMenu(const QPoint &pt)
{
    // Set Hexdump popup menu
    QMenu *menu = ui->hexHexText->createStandardContextMenu();
    menu->clear();
    /*menu->addAction(ui->actionHexCopy_Hexpair);
    menu->addAction(ui->actionHexCopy_ASCII);
    menu->addAction(ui->actionHexCopy_Text);
    menu->addSeparator();*/
    QMenu *colSubmenu = menu->addMenu(tr("Columns"));
    colSubmenu->addAction(ui->action4columns);
    colSubmenu->addAction(ui->action8columns);
    colSubmenu->addAction(ui->action16columns);
    colSubmenu->addAction(ui->action32columns);

    QMenu *formatSubmenu = menu->addMenu(tr("Format"));
    formatSubmenu->addAction(ui->actionFormatHex);
    formatSubmenu->addAction(ui->actionFormatOctal);

    menu->addSeparator();
    syncAction.setText(tr("Sync/unsync offset"));
    menu->addAction(&syncAction);

    // TODO:
    // formatSubmenu->addAction(ui->actionFormatHalfWord);
    // formatSubmenu->addAction(ui->actionFormatWord);
    // formatSubmenu->addAction(ui->actionFormatQuadWord);
    // formatSubmenu->addAction(ui->actionFormatEmoji);

    // TODO:
    // QMenu *signedIntFormatSubmenu = formatSubmenu->addMenu(tr("Signed integer"));
    // signedIntFormatSubmenu->addAction(ui->actionFormatSignedInt1);
    // signedIntFormatSubmenu->addAction(ui->actionFormatSignedInt2);
    // signedIntFormatSubmenu->addAction(ui->actionFormatSignedInt4);

    /*menu->addSeparator();
    menu->addAction(ui->actionHexEdit);
    menu->addAction(ui->actionHexPaste);
    menu->addSeparator();
    menu->addAction(ui->actionHexInsert_Hex);
    menu->addAction(ui->actionHexInsert_String);*/

    ui->hexHexText->setContextMenuPolicy(Qt::CustomContextMenu);

    menu->exec(ui->hexHexText->mapToGlobal(pt));
    delete menu;
}

void HexdumpWidget::toggleSync()
{
    QString windowTitle = tr("Hexdump");
    seekable->toggleSyncWithCore();
    if (seekable->getSyncWithCore()) {
        setWindowTitle(windowTitle);
    } else {
        setWindowTitle(windowTitle + " (not synced)");
        seekable->setIndependentOffset(Core()->getOffset());
    }
}

void HexdumpWidget::showHexASCIIContextMenu(const QPoint &pt)
{
    // Set Hex ASCII popup menu
    QMenu *menu = ui->hexASCIIText->createStandardContextMenu();
    menu->clear();
    /*menu->addAction(ui->actionHexCopy_Hexpair);
    menu->addAction(ui->actionHexCopy_ASCII);
    menu->addAction(ui->actionHexCopy_Text);
    menu->addSeparator();*/
    QMenu *colSubmenu = menu->addMenu("Columns");
    colSubmenu->addAction(ui->action4columns);
    colSubmenu->addAction(ui->action8columns);
    colSubmenu->addAction(ui->action16columns);
    colSubmenu->addAction(ui->action32columns);
    /*menu->addSeparator();
    menu->addAction(ui->actionHexEdit);
    menu->addAction(ui->actionHexPaste);
    menu->addSeparator();
    menu->addAction(ui->actionHexInsert_Hex);
    menu->addAction(ui->actionHexInsert_String);*/

    ui->hexASCIIText->setContextMenuPolicy(Qt::CustomContextMenu);

    menu->exec(ui->hexASCIIText->mapToGlobal(pt));
    delete menu;
}

void HexdumpWidget::setupFonts()
{
    QFont font = Config()->getFont();

    ui->hexOffsetText->setFont(font);
    ui->hexHexText->setFont(font);
    ui->hexASCIIText->setFont(font);

    ui->offsetHeaderLabel->setFont(font);
    ui->hexHeaderLabel->setFont(font);
    ui->asciiHeaderLabel->setFont(font);

    ui->hexDisasTextEdit->setFont(font);
}

void HexdumpWidget::fontsUpdated()
{
    setupFonts();
}

void HexdumpWidget::colorsUpdatedSlot()
{
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

RVA HexdumpWidget::hexPositionToAddress(int position)
{
    switch (format) {

    case Octal:
        return first_loaded_address + (position / 4);
    case Hex:
    default:
        return first_loaded_address + (position / 3);
    }
    return RVA_INVALID;
    // In hex each byte takes up 2 characters + 1 spacer (including newline as spacer)

}

RVA HexdumpWidget::asciiPositionToAddress(int position)
{
    // Each row adds one byte (because of the newline), so cols + 1 gets rid of that offset
    return first_loaded_address + (position - (position / (cols + 1)));
}

int HexdumpWidget::hexAddressToPosition(RVA address)
{
    // This strictly assumes that the address is actually loaded.
    switch (format) {

    case Octal:
        return (address - first_loaded_address) * 4;
    case Hex:
    default:
        return (address - first_loaded_address) * 3;
    }
}

int HexdumpWidget::asciiAddressToPosition(RVA address)
{
    RVA local_address = address - first_loaded_address;
    int position = local_address + (local_address / cols);
    return position;
}

void HexdumpWidget::setTextEditPosition(QTextEdit *textEdit, int position)
{
    QTextCursor textCursor = textEdit->textCursor();
    textCursor.setPosition(position);
    textEdit->setTextCursor(textCursor);
}

int HexdumpWidget::getDisplayedLined(QTextEdit *textEdit, bool bottom)
{
    //int start_pos = textEdit->cursorForPosition(QPoint(0, 0)).position();
    QPoint top_right(textEdit->viewport()->x(), textEdit->viewport()->y());
    QPoint bottom_right(textEdit->viewport()->width(), textEdit->viewport()->height() - 1);
    QPoint point = top_right;
    if (bottom) {
        point = bottom_right;
    }

    QTextCursor textCursor = textEdit->cursorForPosition(point);
    //QTextBlock textBlock = textCursor.block();
    //QTextLayout *textLayout = textBlock.layout();
    //const int relativePos = textCursor.position() - textBlock.position();
    //int end_pos = textEdit->cursorForPosition(bottom_right).position();
    return textCursor.blockNumber();
}

void HexdumpWidget::removeTopLinesWithoutScroll(QTextEdit *textEdit, int lines)
{
    int scroll_val_before = textEdit->verticalScrollBar()->value();
    int height_before = textEdit->document()->size().height();

    QTextBlock block = textEdit->document()->firstBlock();
    for (int i = 0; i < lines; i++) {
        QTextCursor cursor(block);
        block = block.next();
        cursor.movePosition(QTextCursor::NextBlock, QTextCursor::KeepAnchor);
        cursor.removeSelectedText();
    }

    int height_after = textEdit->document()->size().height();
    textEdit->verticalScrollBar()->setValue(scroll_val_before + (height_after - height_before));
}

void HexdumpWidget::removeBottomLinesWithoutScroll(QTextEdit *textEdit, int lines)
{
    QTextBlock block = textEdit->document()->lastBlock().previous();
    QTextCursor textCursor = textEdit->textCursor();
    for (int i = 0; i < lines; i++) {
        QTextCursor cursor(block);
        block = block.previous();
        cursor.select(QTextCursor::BlockUnderCursor);
        cursor.removeSelectedText();
    }
}

void HexdumpWidget::prependWithoutScroll(QTextEdit *textEdit, QString text)
{
    // TODO: Keep selection (already works for append)
    QTextCursor textCursor = textEdit->textCursor();
    int current_positon = textCursor.position();

    //int scroll_max_before = textEdit->verticalScrollBar()->maximum();
    int scroll_val_before = textEdit->verticalScrollBar()->value();
    int height_before = textEdit->document()->size().height();
    textEdit->moveCursor(QTextCursor::Start);
    textEdit->insertPlainText(text);
    textCursor.setPosition(text.length() + current_positon);
    textEdit->setTextCursor(textCursor);
    int height_after = textEdit->document()->size().height();
    //int scroll_max_after = textEdit->verticalScrollBar()->maximum();
    //int scroll_val_after = textEdit->verticalScrollBar()->maximum();
    textEdit->verticalScrollBar()->setValue(scroll_val_before + (height_after - height_before));
}

void HexdumpWidget::appendWithoutScroll(QTextEdit *textEdit, QString text)
{
    int scroll_val_before = textEdit->verticalScrollBar()->value();
    QTextCursor textCursor = textEdit->textCursor();
    textEdit->moveCursor(QTextCursor::End);
    textEdit->insertPlainText(text);
    textEdit->setTextCursor(textCursor);
    textEdit->verticalScrollBar()->setValue(scroll_val_before);
}

void HexdumpWidget::scrollChanged()
{
    connectScroll(true);

    int firstLine = getDisplayedLined(ui->hexHexText);
    if (firstLine < (bufferLines / 2)) {
        int loadLines = bufferLines;
        RVA shift = static_cast<RVA>(loadLines * cols);
        if (shift > first_loaded_address) {
            loadLines = static_cast<int>(first_loaded_address / cols);
            shift = first_loaded_address;
        }
        first_loaded_address -= shift;
        last_loaded_address -= shift;

        if (loadLines > 0) {
            auto hexdump = fetchHexdump(first_loaded_address, loadLines);
            prependWithoutScroll(ui->hexOffsetText, hexdump[0]);
            prependWithoutScroll(ui->hexHexText, hexdump[1]);
            prependWithoutScroll(ui->hexASCIIText, hexdump[2]);

            removeBottomLinesWithoutScroll(ui->hexOffsetText, loadLines);
            removeBottomLinesWithoutScroll(ui->hexHexText, loadLines);
            removeBottomLinesWithoutScroll(ui->hexASCIIText, loadLines);

            ui->hexOffsetText->verticalScrollBar()->setValue(ui->hexHexText->verticalScrollBar()->value());
            ui->hexASCIIText->verticalScrollBar()->setValue(ui->hexHexText->verticalScrollBar()->value());
        }
    }

    int blocks  = ui->hexHexText->document()->blockCount();
    int lastLine = getDisplayedLined(ui->hexHexText, true);
    if (blocks - lastLine < (bufferLines / 2)) {
        int loadLines = bufferLines;
        RVA shift = static_cast<RVA>(loadLines * cols);
        if (last_loaded_address > RVA_MAX - shift) {
            shift = RVA_MAX - last_loaded_address;
            loadLines = static_cast<int>(shift / cols);
        }

        if (loadLines > 0) {
            auto hexdump = fetchHexdump(last_loaded_address, loadLines);
            last_loaded_address += shift;
            first_loaded_address += shift;

            removeTopLinesWithoutScroll(ui->hexOffsetText, loadLines);
            removeTopLinesWithoutScroll(ui->hexHexText, loadLines);
            removeTopLinesWithoutScroll(ui->hexASCIIText, loadLines);
            appendWithoutScroll(ui->hexOffsetText, hexdump[0]);
            appendWithoutScroll(ui->hexHexText, hexdump[1]);
            appendWithoutScroll(ui->hexASCIIText, hexdump[2]);
        }
    }
    connectScroll(false);
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

void HexdumpWidget::on_action8columns_triggered()
{
    Core()->setConfig("hex.cols", 8);
    refresh();
}

void HexdumpWidget::on_action16columns_triggered()
{
    Core()->setConfig("hex.cols", 16);
    refresh();
}

void HexdumpWidget::on_action4columns_triggered()
{
    Core()->setConfig("hex.cols", 4);
    refresh();
}

void HexdumpWidget::on_action32columns_triggered()
{
    Core()->setConfig("hex.cols", 32);
    refresh();
}

void HexdumpWidget::on_action64columns_triggered()
{
    Core()->setConfig("hex.cols", 64);
    refresh();
}

void HexdumpWidget::on_action2columns_triggered()
{
    Core()->setConfig("hex.cols", 2);
    refresh();
}

void HexdumpWidget::on_action1column_triggered()
{
    Core()->setConfig("hex.cols", 1);
    refresh();
}

void HexdumpWidget::on_actionFormatHex_triggered()
{
    format = Format::Hex;
    refresh();
}

void HexdumpWidget::on_actionFormatOctal_triggered()
{
    format = Format::Octal;
    refresh();
}

void HexdumpWidget::on_parseTypeComboBox_currentTextChanged(const QString &)
{
    if (ui->parseTypeComboBox->currentIndex() == 0) {
        ui->hexSideFrame_2->show();
    } else {
        ui->hexSideFrame_2->hide();
    }
    selectionChanged();
}

void HexdumpWidget::on_parseEndianComboBox_currentTextChanged(const QString &)
{
    selectionChanged();
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

void HexdumpWidget::wheelEvent(QWheelEvent *event)
{
    if ( Qt::ControlModifier == event->modifiers() ) {
        const QPoint numDegrees = event->angleDelta() / 8;
        if (!numDegrees.isNull()) {
            const QPoint numSteps = numDegrees / 15;
            if ( 0 != numSteps.y() ) {
                if (numSteps.y() > 0) {
                    zoomIn(1);
                } else if ( numSteps.y() < 0 ) {
                    zoomOut(1);
                }
            }
        }
        event->accept();
        return;
    }

    event->ignore();
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
    QString arch = Core()->cmd("e asm.arch").trimmed();
    QString bits = Core()->cmd("e asm.bits").trimmed();

    //int arch_index = ui->hexArchComboBox_2->findText(arch);
    if (ui->parseArchComboBox->findText(arch) != -1) {
        ui->parseArchComboBox->setCurrentIndex(ui->parseArchComboBox->findText(arch));
    }

    //int bits_index = ui->hexBitsComboBox_2->findText(bits);
    if (ui->parseBitsComboBox->findText(bits) != -1) {
        ui->parseBitsComboBox->setCurrentIndex(ui->parseBitsComboBox->findText(bits));
    }
}

void HexdumpWidget::showOffsets(bool show)
{
    if (show) {
        ui->hexOffsetText->show();
        Core()->setConfig("asm.offset", 1);
    } else {
        ui->hexOffsetText->hide();
        Core()->setConfig("asm.offset", 0);
    }
}

void HexdumpWidget::zoomIn(int range)
{
    ui->hexOffsetText->zoomIn(range);
    ui->hexASCIIText->zoomIn(range);
    ui->hexHexText->zoomIn(range);

    updateWidths();
}

void HexdumpWidget::zoomOut(int range)
{
    ui->hexOffsetText->zoomOut(range);
    ui->hexASCIIText->zoomOut(range);
    ui->hexHexText->zoomOut(range);

    updateWidths();
}

void HexdumpWidget::updateWidths()
{
    // Update width
    ui->hexHexText->document()->adjustSize();
    ui->hexHexText->setFixedWidth(ui->hexHexText->document()->size().width());

    ui->hexOffsetText->document()->adjustSize();
    ui->hexOffsetText->setFixedWidth(ui->hexOffsetText->document()->size().width());

    ui->hexASCIIText->document()->adjustSize();
    ui->hexASCIIText->setMinimumWidth(ui->hexASCIIText->document()->size().width());
}
