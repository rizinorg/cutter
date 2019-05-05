#include "HexTextView.h"
#include "ui_HexTextView.h"

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

HexTextView::HexTextView(QWidget *parent) :
    QScrollArea(parent),
    ui(new Ui::HexTextView)
{
    ui->setupUi(this);

    // Setup hex highlight
    //connect(ui->hexHexText, SIGNAL(cursorPositionChanged()), this, SLOT(highlightHexCurrentLine()));
    //highlightHexCurrentLine();

    int margin = static_cast<int>(ui->hexOffsetText->document()->documentMargin());
    ui->offsetHeaderLabel->setContentsMargins(margin, 0, margin, 0);

    margin = static_cast<int>(ui->hexHexText->document()->documentMargin());
    ui->hexHeaderLabel->setContentsMargins(margin, 0, margin, 0);

    margin = static_cast<int>(ui->hexASCIIText->document()->documentMargin());
    ui->asciiHeaderLabel->setContentsMargins(margin, 0, margin, 0);

    setupFonts();

    updateHeaders();

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

    connect(ui->hexHexText, &QTextEdit::selectionChanged, this, &HexTextView::onSelectionChanged);
    connect(ui->hexASCIIText, &QTextEdit::selectionChanged, this, &HexTextView::onSelectionChanged);
    connect(ui->hexHexText, &QTextEdit::cursorPositionChanged, this, &HexTextView::onSelectionChanged);
    connect(ui->hexASCIIText, &QTextEdit::cursorPositionChanged, this,
            &HexTextView::onSelectionChanged);
    connect(&rangeDialog, &QDialog::accepted, this, &HexTextView::on_rangeDialogAccepted);

    addAction(ui->actionResetZoom);
    connect(ui->actionResetZoom, &QAction::triggered, this, &HexTextView::zoomReset);
    defaultFontSize = ui->hexHexText->font().pointSizeF();

    addAction(ui->actionCopyAddressAtCursor);
    ui->actionCopyAddressAtCursor->setShortcut(Qt::CTRL + Qt::SHIFT + Qt::Key_C);

    format = Format::Hex;
}

void HexTextView::setupScrollSync()
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

void HexTextView::connectScroll(bool disconnect_)
{
    scroll_disabled = disconnect_;
    if (disconnect_) {
        disconnect(ui->hexHexText->verticalScrollBar(), &QScrollBar::valueChanged, this,
                   &HexTextView::scrollChanged);
        disconnect(ui->hexHexText, &QTextEdit::cursorPositionChanged, this, &HexTextView::scrollChanged);
    } else {
        connect(ui->hexHexText->verticalScrollBar(), &QScrollBar::valueChanged, this,
                &HexTextView::scrollChanged);
        connect(ui->hexHexText, &QTextEdit::cursorPositionChanged, this, &HexTextView::scrollChanged);

    }
}

HexTextView::~HexTextView() {}

/*
 * Text highlight functions
 * Currently unused
 */
/*
void HexTextView::highlightHexCurrentLine()
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


void HexTextView::highlightHexWords(const QString &str)
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

void HexTextView::refresh(RVA addr)
{
    if (addr == RVA_INVALID) {
        addr = currentPos;
    } else {
        currentPos = addr;
    }

    ut64 loadLines = 0;
    ut64 curAddrLineOffset = 0;
    connectScroll(true);

    updateHeaders();



    cols = Core()->getConfigi("hex.cols");
    // Avoid divison by 0
    if (cols == 0)
        cols = 16;

    // TODO: Figure out how to calculate a sane value for this
    bufferLines = qhelpers::getMaxFullyDisplayedLines(ui->hexHexText) * 10;

    if (requestedSelectionEndAddress != 0 && requestedSelectionStartAddress != 0
            && requestedSelectionEndAddress > requestedSelectionStartAddress) {
        loadLines = ((requestedSelectionEndAddress - requestedSelectionStartAddress) / cols) +
                    (bufferLines * 2);
        curAddrLineOffset = bufferLines;
    } else {
        loadLines = bufferLines * 3; // total lines to load
        curAddrLineOffset = bufferLines; // line number where seek should be
    }

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

void HexTextView::updateHeaders()
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


std::array<QString, 3> HexTextView::fetchHexdump(RVA addr, int lines)
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

void HexTextView::onSelectionChanged()
{
    if (scroll_disabled) {
        return;
    }
    connectScroll(true);

    if (sender() == ui->hexHexText) {
        QTextCursor textCursor = ui->hexHexText->textCursor();
        if (!textCursor.hasSelection()) {
            RVA adr = hexPositionToAddress(textCursor.position());
            int pos = asciiAddressToPosition(adr);
            setTextEditPosition(ui->hexASCIIText, pos);

            selection = {true, 0, 0};
            emit selectionChanged(selection);
            currentPos = adr;
            emit positionChanged(adr);
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

        int startPosition = asciiAddressToPosition(startAddress);
        int endPosition = asciiAddressToPosition(endAddress);
        QTextCursor targetTextCursor = ui->hexASCIIText->textCursor();
        targetTextCursor.setPosition(startPosition);
        targetTextCursor.setPosition(endPosition, QTextCursor::KeepAnchor);
        ui->hexASCIIText->setTextCursor(targetTextCursor);

        selection = {false, startAddress, endAddress > startAddress ? endAddress - 1 : endAddress};
        emit selectionChanged(selection);
        currentPos = startAddress;
        emit positionChanged(startAddress);
    } else {
        QTextCursor textCursor = ui->hexASCIIText->textCursor();
        if (!textCursor.hasSelection()) {
            RVA adr = asciiPositionToAddress(textCursor.position());
            int pos = hexAddressToPosition(adr);
            setTextEditPosition(ui->hexHexText, pos);
            connectScroll(false);
            selection = {false, 0, 0};
            emit selectionChanged(selection);
            currentPos = adr;
            emit positionChanged(adr);
            return;
        }
        RVA startAddress = asciiPositionToAddress(textCursor.selectionStart());
        RVA endAddress = asciiPositionToAddress(textCursor.selectionEnd());

        int startPosition = hexAddressToPosition(startAddress);
        int endPosition = hexAddressToPosition(endAddress);

        // End position -1 because the position we get above is for the next
        // entry, so including the space/newline
        endPosition -= 1;
        QTextCursor targetTextCursor = ui->hexHexText->textCursor();
        targetTextCursor.setPosition(startPosition);
        targetTextCursor.setPosition(endPosition, QTextCursor::KeepAnchor);
        ui->hexHexText->setTextCursor(targetTextCursor);

        selection = {false, startAddress, endAddress > startAddress ? endAddress - 1 : endAddress};
        emit selectionChanged(selection);
        currentPos = startAddress;
        emit positionChanged(startAddress);
    }

    connectScroll(false);
    return;
}



void HexTextView::showHexdumpContextMenu(const QPoint &pt)
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

    menu->addAction(ui->actionSelect_Block);

    menu->addSeparator();
    menu->addActions(this->actions());

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

void HexTextView::showHexASCIIContextMenu(const QPoint &pt)
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

void HexTextView::setupFonts()
{
    QFont font = Config()->getFont();

    ui->hexOffsetText->setFont(font);
    ui->hexHexText->setFont(font);
    ui->hexASCIIText->setFont(font);

    ui->offsetHeaderLabel->setFont(font);
    ui->hexHeaderLabel->setFont(font);
    ui->asciiHeaderLabel->setFont(font);
}

HexTextView::Selection HexTextView::getSelection()
{
    return selection;
}

RVA HexTextView::position()
{
    return currentPos;
}

void HexTextView::fontsUpdated()
{
    setupFonts();
}


RVA HexTextView::hexPositionToAddress(int position)
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

RVA HexTextView::asciiPositionToAddress(int position)
{
    // Each row adds one byte (because of the newline), so cols + 1 gets rid of that offset
    return first_loaded_address + (position - (position / (cols + 1)));
}

int HexTextView::hexAddressToPosition(RVA address)
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

int HexTextView::asciiAddressToPosition(RVA address)
{
    RVA local_address = address - first_loaded_address;
    int position = local_address + (local_address / cols);
    return position;
}

void HexTextView::setTextEditPosition(QTextEdit *textEdit, int position)
{
    QTextCursor textCursor = textEdit->textCursor();
    textCursor.setPosition(position);
    textEdit->setTextCursor(textCursor);
}

int HexTextView::getDisplayedLined(QTextEdit *textEdit, bool bottom)
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

void HexTextView::removeTopLinesWithoutScroll(QTextEdit *textEdit, int lines)
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

void HexTextView::removeBottomLinesWithoutScroll(QTextEdit *textEdit, int lines)
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

void HexTextView::prependWithoutScroll(QTextEdit *textEdit, QString text)
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

void HexTextView::appendWithoutScroll(QTextEdit *textEdit, QString text)
{
    int scroll_val_before = textEdit->verticalScrollBar()->value();
    QTextCursor textCursor = textEdit->textCursor();
    textEdit->moveCursor(QTextCursor::End);
    textEdit->insertPlainText(text);
    textEdit->setTextCursor(textCursor);
    textEdit->verticalScrollBar()->setValue(scroll_val_before);
}

void HexTextView::scrollChanged()
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

void HexTextView::on_actionCopyAddressAtCursor_triggered()
{
    auto addr = hexPositionToAddress(ui->hexHexText->textCursor().position());

    QClipboard *clipboard = QApplication::clipboard();
    clipboard->setText(RAddressString(addr));
}


void HexTextView::on_action8columns_triggered()
{
    Core()->setConfig("hex.cols", 8);
    refresh();
}

void HexTextView::on_action16columns_triggered()
{
    Core()->setConfig("hex.cols", 16);
    refresh();
}

void HexTextView::on_action4columns_triggered()
{
    Core()->setConfig("hex.cols", 4);
    refresh();
}

void HexTextView::on_action32columns_triggered()
{
    Core()->setConfig("hex.cols", 32);
    refresh();
}

void HexTextView::on_action64columns_triggered()
{
    Core()->setConfig("hex.cols", 64);
    refresh();
}

void HexTextView::on_action2columns_triggered()
{
    Core()->setConfig("hex.cols", 2);
    refresh();
}

void HexTextView::on_action1column_triggered()
{
    Core()->setConfig("hex.cols", 1);
    refresh();
}

void HexTextView::on_actionFormatHex_triggered()
{
    format = Format::Hex;
    refresh();
}

void HexTextView::on_actionFormatOctal_triggered()
{
    format = Format::Octal;
    refresh();
}

void HexTextView::on_actionSelect_Block_triggered()
{

    //get the current hex address from current cursor location
    rangeDialog.setStartAddress(
        hexPositionToAddress(ui->hexHexText->textCursor().position()));
    rangeDialog.setModal(false);
    rangeDialog.show();
    rangeDialog.activateWindow();
    rangeDialog.raise();

}



void HexTextView::resizeEvent(QResizeEvent *event)
{
    QScrollArea::resizeEvent(event);
    refresh();
}

void HexTextView::wheelEvent(QWheelEvent *event)
{
    if ( Qt::ControlModifier == event->modifiers() ) {
        const QPoint numDegrees = event->angleDelta() / 8;
        if (!numDegrees.isNull()) {
            const QPoint numSteps = numDegrees / 15;
            if ( 0 != numSteps.y() ) {
                zoomIn(numSteps.y() > 0 ? 1 : -1);
            }
        }
        event->accept();
        return;
    }

    event->ignore();
}



void HexTextView::on_rangeDialogAccepted()
{
    int                 startPosition;
    int                 endPosition;
    QTextCursor         targetTextCursor;

    requestedSelectionStartAddress = Core()->math(rangeDialog.getStartAddress());
    requestedSelectionEndAddress = rangeDialog.getEndAddressRadioButtonChecked() ?
                                   Core()->math(rangeDialog.getEndAddress()) :
                                   requestedSelectionStartAddress + Core()->math(rangeDialog.getLength());

    //not sure what the accepted user feedback mechanism is, output to console or a QMessageBox alert
    if (requestedSelectionEndAddress <= requestedSelectionStartAddress) {
        Core()->message(tr("Error: Could not select range, end address is less then start address"));
        return;
    }

    //seek to the start address and create a text cursor to highlight the desired range
    refresh(requestedSelectionStartAddress);

    //for large selections, won't be able to calculate the endPosition because hexAddressToPosition assumes the address is loaded?
    startPosition = hexAddressToPosition(requestedSelectionStartAddress);
    endPosition = hexAddressToPosition(requestedSelectionEndAddress) - 1;

    targetTextCursor = ui->hexHexText->textCursor();

    targetTextCursor.setPosition(startPosition);
    targetTextCursor.setPosition(endPosition, QTextCursor::KeepAnchor);

    ui->hexHexText->setTextCursor(targetTextCursor);
}

void HexTextView::showOffsets(bool show)
{
    if (show) {
        ui->hexOffsetText->show();
        Core()->setConfig("asm.offset", 1);
    } else {
        ui->hexOffsetText->hide();
        Core()->setConfig("asm.offset", 0);
    }
}

void HexTextView::zoomIn(int range)
{
    ui->hexHexText->zoomIn(range);
    syncScale();
}

void HexTextView::zoomOut(int range)
{
    zoomIn(-range);
}

void HexTextView::zoomReset()
{
    QFont font(ui->hexHexText->font());
    font.setPointSizeF(defaultFontSize);
    ui->hexHexText->setFont(font);
    syncScale();
}

void HexTextView::updateWidths()
{
    // Update width
    auto idealWidth = ui->hexHexText->document()->idealWidth();
    ui->hexHexText->document()->setTextWidth(idealWidth);

    ui->hexOffsetText->document()->adjustSize();
    ui->hexOffsetText->setFixedWidth(ui->hexOffsetText->document()->size().width() + 1);

    ui->hexASCIIText->document()->adjustSize();
    ui->hexASCIIText->setMinimumWidth(ui->hexASCIIText->document()->size().width());
}

void HexTextView::syncScale()
{
    ui->hexOffsetText->setFont(ui->hexHexText->font());
    ui->hexASCIIText->setFont(ui->hexHexText->font());
    ui->offsetHeaderLabel->setFont(ui->hexHexText->font());
    ui->hexHeaderLabel->setFont(ui->hexHexText->font());
    ui->asciiHeaderLabel->setFont(ui->hexHexText->font());
    updateWidths();
}
