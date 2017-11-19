
#include "HexdumpWidget.h"
#include "ui_HexdumpWidget.h"
#include "DisassemblerGraphView.h"

#include "MainWindow.h"
#include "utils/Helpers.h"

#include <QTemporaryFile>
#include <QFontDialog>
#include <QScrollBar>
#include <QClipboard>
#include <QShortcut>
#include <QMenu>
#include <QFont>
#include <QUrl>
#include <QSettings>

#include <cassert>

const int HexdumpWidget::linesMarginMin = 32;
const int HexdumpWidget::linesMarginDefault = 48;
const int HexdumpWidget::linesMarginMax = 64;

HexdumpWidget::HexdumpWidget(QWidget *parent, Qt::WindowFlags flags) :
        QDockWidget(parent, flags),
        ui(new Ui::HexdumpWidget)
{
    ui->setupUi(this);

    //this->on_actionSettings_menu_1_triggered();

    // Setup hex highlight
    //connect(ui->hexHexText, SIGNAL(cursorPositionChanged()), this, SLOT(highlightHexCurrentLine()));
    //highlightHexCurrentLine();

    // Normalize fonts for other OS
    qhelpers::normalizeFont(ui->hexOffsetText);
    qhelpers::normalizeFont(ui->hexHexText);
    qhelpers::normalizeFont(ui->hexASCIIText);

    // Popup menu on Settings toolbutton
    QMenu *memMenu = new QMenu();
    ui->memSettingsButton_2->addAction(ui->actionSettings_menu_1);
    memMenu->addAction(ui->actionSettings_menu_1);
    ui->memSettingsButton_2->setMenu(memMenu);

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

    connect(Core(), SIGNAL(seekChanged(RVA)), this, SLOT(on_seekChanged(RVA)));
    connect(Core(), SIGNAL(raisePrioritizedMemoryWidget(CutterCore::MemoryWidgetType)), this, SLOT(raisePrioritizedMemoryWidget(CutterCore::MemoryWidgetType)));

    connect(this, &QDockWidget::visibilityChanged, this, [](bool visibility) {
        if (visibility)
        {
            Core()->setMemoryWidgetPriority(CutterCore::MemoryWidgetType::Hexdump);
        }
    });

    connect(Core(), &CutterCore::refreshAll, this, [this]() {
        refresh(Core()->getOffset());
    });

    fillPlugins();
}

HexdumpWidget::HexdumpWidget(const QString &title, QWidget *parent, Qt::WindowFlags flags)
        : HexdumpWidget(parent, flags)
{
    setWindowTitle(title);
}


void HexdumpWidget::setupScrollSync()
{
    /*
     * For some reason, QScrollBar::valueChanged is not emitted when
     * the scrolling happened from moving the cursor beyond the visible content,
     * so QPlainTextEdit::cursorPositionChanged has to be connected as well.
     */

    auto offsetHexFunc = [this]() {
        ui->hexHexText->verticalScrollBar()->setValue(ui->hexOffsetText->verticalScrollBar()->value());
    };

    auto offsetASCIIFunc = [this]() {
        ui->hexASCIIText->verticalScrollBar()->setValue(ui->hexOffsetText->verticalScrollBar()->value());
    };

    connect(ui->hexOffsetText->verticalScrollBar(), &QScrollBar::valueChanged, ui->hexHexText->verticalScrollBar(), offsetHexFunc);
    connect(ui->hexOffsetText, &QPlainTextEdit::cursorPositionChanged, ui->hexHexText->verticalScrollBar(), offsetHexFunc);
    connect(ui->hexOffsetText->verticalScrollBar(), &QScrollBar::valueChanged, ui->hexASCIIText->verticalScrollBar(), offsetASCIIFunc);
    connect(ui->hexOffsetText, &QPlainTextEdit::cursorPositionChanged, ui->hexASCIIText->verticalScrollBar(), offsetASCIIFunc);


    auto hexOffsetFunc = [this]() {
        ui->hexOffsetText->verticalScrollBar()->setValue(ui->hexHexText->verticalScrollBar()->value());
    };

    auto hexASCIIFunc = [this]() {
        ui->hexASCIIText->verticalScrollBar()->setValue(ui->hexHexText->verticalScrollBar()->value());
    };

    connect(ui->hexHexText->verticalScrollBar(), &QScrollBar::valueChanged, ui->hexOffsetText->verticalScrollBar(), hexOffsetFunc);
    connect(ui->hexHexText, &QPlainTextEdit::cursorPositionChanged, ui->hexOffsetText->verticalScrollBar(), hexOffsetFunc);
    connect(ui->hexHexText->verticalScrollBar(), &QScrollBar::valueChanged, ui->hexASCIIText->verticalScrollBar(), hexASCIIFunc);
    connect(ui->hexHexText, &QPlainTextEdit::cursorPositionChanged, ui->hexASCIIText->verticalScrollBar(), hexASCIIFunc);


    auto asciiOffsetFunc = [this]() {
        ui->hexOffsetText->verticalScrollBar()->setValue(ui->hexASCIIText->verticalScrollBar()->value());
    };

    auto asciiHexFunc = [this]() {
        ui->hexHexText->verticalScrollBar()->setValue(ui->hexASCIIText->verticalScrollBar()->value());
    };

    connect(ui->hexASCIIText->verticalScrollBar(), &QScrollBar::valueChanged, ui->hexOffsetText->verticalScrollBar(), asciiOffsetFunc);
    connect(ui->hexASCIIText, &QPlainTextEdit::cursorPositionChanged, ui->hexOffsetText->verticalScrollBar(), asciiOffsetFunc);
    connect(ui->hexASCIIText->verticalScrollBar(), &QScrollBar::valueChanged, ui->hexHexText->verticalScrollBar(), asciiHexFunc);
    connect(ui->hexASCIIText, &QPlainTextEdit::cursorPositionChanged, ui->hexHexText->verticalScrollBar(), asciiHexFunc);
}


void HexdumpWidget::on_seekChanged(RVA addr)
{
    refresh(addr);
}


void HexdumpWidget::raisePrioritizedMemoryWidget(CutterCore::MemoryWidgetType type)
{
    if (type == CutterCore::MemoryWidgetType::Hexdump)
    {
        raise();
    }
}

void HexdumpWidget::connectScroll(bool disconnect)
{
    if (disconnect)
    {
        this->disconnect(ui->hexASCIIText->verticalScrollBar(), &QScrollBar::valueChanged, this,
                         &HexdumpWidget::adjustHexdumpLines);
        this->disconnect(ui->hexASCIIText, &QPlainTextEdit::cursorPositionChanged, this,
                         &HexdumpWidget::adjustHexdumpLines);
    }
    else
    {
        connect(ui->hexASCIIText->verticalScrollBar(), &QScrollBar::valueChanged, this,
                &HexdumpWidget::adjustHexdumpLines);
        connect(ui->hexASCIIText, &QPlainTextEdit::cursorPositionChanged, this, &HexdumpWidget::adjustHexdumpLines);
    }
}

HexdumpWidget::~HexdumpWidget() {}

/*
 * Text highlight functions
 */
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

void HexdumpWidget::refresh(RVA addr)
{
    if (addr == RVA_INVALID)
    {
        addr = Core()->getOffset();
    }

    int visibleLines = qhelpers::getMaxFullyDisplayedLines(ui->hexHexText);

    RCoreLocked lcore = Core()->core();

    connectScroll(true);


    int cols = lcore->print->cols;
    RVA marginBytes = static_cast<RVA>(linesMarginDefault) * cols;

    // lower bound of 0
    if (addr > marginBytes)
    {
        topOffset = addr - marginBytes;
        topOffset = (topOffset / cols) * cols; // align
    }
    else
    {
        topOffset = 0;
    }


    int fetchLines = visibleLines + linesMarginDefault * 2;
    RVA bytes = static_cast<RVA>(fetchLines) * cols;


    // upper bound of UT64_MAX
    RVA bytesLeft = UT64_MAX - topOffset;
    if (bytes > bytesLeft)
    {
        bottomOffset = UT64_MAX;
        topOffset = bottomOffset - bytes;
    }
    else
    {
        bottomOffset = topOffset + bytes;
    }


    auto hexdump = fetchHexdump(topOffset, bytes);

    ui->hexOffsetText->setPlainText(hexdump[0]);
    ui->hexHexText->setPlainText(hexdump[1]);
    ui->hexASCIIText->setPlainText(hexdump[2]);
    resizeHexdump();


    int seekLine = static_cast<int>((addr - topOffset) / cols);

    // Move cursors to desired address
    QTextCursor cur = ui->hexOffsetText->textCursor();
    cur.movePosition(QTextCursor::Start);
    cur.movePosition(QTextCursor::Down, QTextCursor::MoveAnchor, seekLine);
    ui->hexOffsetText->setTextCursor(cur);

    cur = ui->hexHexText->textCursor();
    cur.movePosition(QTextCursor::Start);
    cur.movePosition(QTextCursor::Down, QTextCursor::MoveAnchor, seekLine);
    ui->hexHexText->setTextCursor(cur);

    cur = ui->hexASCIIText->textCursor();
    cur.movePosition(QTextCursor::Start);
    cur.movePosition(QTextCursor::Down, QTextCursor::MoveAnchor, seekLine);
    ui->hexASCIIText->setTextCursor(cur);

    ui->hexOffsetText->verticalScrollBar()->setValue(seekLine);
    ui->hexHexText->verticalScrollBar()->setValue(seekLine);
    ui->hexASCIIText->verticalScrollBar()->setValue(seekLine);

    connectScroll(false);
}

void HexdumpWidget::appendHexdumpLines(int lines, bool top)
{
    connectScroll(true);

    int cols = Core()->getConfigi("hex.cols");
    RVA bytes = static_cast<RVA>(lines) * cols;

    if (top)
    {
        if (bytes > topOffset)
        {
            bytes = topOffset;
            if (bytes == 0)
            {
                connectScroll(false);
                return;
            }
        }

        topOffset -= bytes;
        auto hexdump = fetchHexdump(topOffset, bytes);

        int scroll = ui->hexASCIIText->verticalScrollBar()->value();

        QTextCursor cur = ui->hexOffsetText->textCursor();
        cur.movePosition(QTextCursor::Start);
        cur.insertText(hexdump[0]);

        cur = ui->hexHexText->textCursor();
        cur.movePosition(QTextCursor::Start);
        cur.insertText(hexdump[1]);

        cur = ui->hexASCIIText->textCursor();
        cur.movePosition(QTextCursor::Start);
        cur.insertText(hexdump[2]);

        int actualLines = static_cast<int>(bytes / cols);
        ui->hexOffsetText->verticalScrollBar()->setValue(actualLines + scroll);
        ui->hexHexText->verticalScrollBar()->setValue(actualLines + scroll);
        ui->hexASCIIText->verticalScrollBar()->setValue(actualLines + scroll);
    }
    else
    {
        if (bytes > UT64_MAX - bottomOffset)
        {
            bytes = UT64_MAX - bottomOffset;

            if (bytes == 0)
            {
                connectScroll(false);
                return;
            }
        }

        auto hexdump = fetchHexdump(bottomOffset, bytes);
        bottomOffset += bytes;

        QTextCursor cur = ui->hexOffsetText->textCursor();
        cur.movePosition(QTextCursor::End);
        cur.insertText(hexdump[0]);

        cur = ui->hexHexText->textCursor();
        cur.movePosition(QTextCursor::End);
        cur.insertText(hexdump[1]);

        cur = ui->hexASCIIText->textCursor();
        cur.movePosition(QTextCursor::End);
        cur.insertText(hexdump[2]);
    }

    connectScroll(false);
}

void HexdumpWidget::removeHexdumpLines(int lines, bool top)
{
    connectScroll(true);

    int cols = Core()->getConfigi("hex.cols");

	std::array<QPlainTextEdit *, 3> edits = { ui->hexOffsetText, ui->hexHexText, ui->hexASCIIText };

    int scroll = ui->hexASCIIText->verticalScrollBar()->value();

    if (top)
    {
        for (QPlainTextEdit *edit : edits)
        {
            QTextCursor cur = edit->textCursor();
            cur.movePosition(QTextCursor::Start);
            cur.movePosition(QTextCursor::Down, QTextCursor::KeepAnchor, lines + 1);
            cur.removeSelectedText();
        }

        topOffset += lines * cols;

        ui->hexOffsetText->verticalScrollBar()->setValue(scroll - lines);
        ui->hexHexText->verticalScrollBar()->setValue(scroll - lines);
        ui->hexASCIIText->verticalScrollBar()->setValue(scroll - lines);
    }
    else
    {
        for (QPlainTextEdit *edit : edits)
        {
            QTextCursor cur = edit->textCursor();
            cur.movePosition(QTextCursor::End);
            cur.movePosition(QTextCursor::Up, QTextCursor::KeepAnchor, lines);
            cur.movePosition(QTextCursor::StartOfLine, QTextCursor::KeepAnchor);
            cur.removeSelectedText();
        }

        bottomOffset -= lines * cols;
    }

    connectScroll(false);
}

/*
 * Content management functions
 */

void HexdumpWidget::fillPlugins()
{
    // Fill the plugins combo for the hexdump sidebar
    ui->hexArchComboBox_2->insertItems(0, Core()->getAsmPluginNames());
}

std::array<QString, 3> HexdumpWidget::fetchHexdump(RVA offset, RVA bytes)
{
    QString hexdump = Core()->cmd(QString("px %1 @ %2").arg(QString::number(bytes), QString::number(offset)));

    QString offsets;
    QString hex;
    QString ascii;
    int ln = 0;

    for (const QString &line : hexdump.split("\n"))
    {
        if (ln++ == 0 || line.trimmed().isEmpty())
        {
            continue;
        }

        int wc = 0;
        for (const QString a : line.split("  "))
        {
            switch (wc++)
            {
                case 0:
                    offsets += a + "\n";
                    break;
                case 1:
                    hex += a.trimmed() + "\n";
                    break;
                case 2:
                    ascii += a + "\n";
                    break;
            }
        }
    }

    return { offsets, hex, ascii };
}

void HexdumpWidget::resizeHexdump()
{
    ui->hexOffsetText->setMinimumWidth(static_cast<int>(ui->hexOffsetText->document()->size().width()));
    ui->hexHexText->setMinimumWidth(static_cast<int>(ui->hexHexText->document()->size().width()));

    //this->hexASCIIText->setMinimumWidth(this->hexASCIIText->document()->size().width());
}

void HexdumpWidget::adjustHexdumpLines()
{
    QScrollBar *sb = ui->hexASCIIText->verticalScrollBar();
	int topMargin = sb->value() - sb->minimum();
    int bottomMargin = sb->maximum() - sb->value();

    if (topMargin < linesMarginMin)
    {
        int loadLines = linesMarginDefault - topMargin;
        appendHexdumpLines(loadLines, true);
    }

	if(bottomMargin < linesMarginMin)
	{
		int loadLines = linesMarginDefault - bottomMargin;
        appendHexdumpLines(loadLines, false);
	}

    if(topMargin > linesMarginMax)
    {
        int removeLines = topMargin - linesMarginDefault;
        removeHexdumpLines(removeLines, true);
    }

    if(bottomMargin > linesMarginMax)
    {
        int removeLines = bottomMargin - linesMarginDefault;
        removeHexdumpLines(removeLines, false);
    }
}

void HexdumpWidget::on_hexHexText_selectionChanged()
{
    // Get selected partsing type
    QString parsing = ui->codeCombo_2->currentText();
    // Get selected text
    QTextCursor cursor(ui->hexHexText->textCursor());
    QString sel_text = cursor.selectedText();

    sel_text = sel_text.simplified().remove(" ");
    //eprintf ("-- (((%s))) --\n", sel_text.toUtf8().constData());

    if (sel_text == "")
    {
        ui->hexDisasTextEdit->setPlainText("");
        ui->bytesEntropy->setText("");
        ui->bytesMD5->setText("");
        ui->bytesSHA1->setText("");
    }
    else
    {
        if (parsing == "Dissasembly")
        {
            // Get selected combos
            QString arch = ui->hexArchComboBox_2->currentText();
            QString bits = ui->hexBitsComboBox_2->currentText();

            QString oarch = Core()->getConfig("asm.arch");
            QString obits = Core()->getConfig("asm.bits");

            Core()->setConfig("asm.arch", arch);
            Core()->setConfig("asm.bits", bits);
            QString str = Core()->cmd("pad " + sel_text);
            ui->hexDisasTextEdit->setPlainText(str);
            Core()->setConfig("asm.arch", oarch);
            Core()->setConfig("asm.bits", obits);
            //qDebug() << "Selected Arch: " << arch;
            //qDebug() << "Selected Bits: " << bits;
            //qDebug() << "Selected Text: " << sel_text;
        }
            // TODO: update on selection changes.. use cmd("pc "+len+"@"+off)
        else if (parsing == "C byte array")
        {
            ui->hexDisasTextEdit->setPlainText(Core()->cmd("pc@x:" + sel_text));
        }
        else    if (parsing == "C dword array")
        {
            ui->hexDisasTextEdit->setPlainText(Core()->cmd("pcw@x:" + sel_text));
        }
        else    if (parsing == "C qword array")
        {
            ui->hexDisasTextEdit->setPlainText(Core()->cmd("pcq@x:" + sel_text));
        }
        else    if (parsing == "Assembler")
        {
            ui->hexDisasTextEdit->setPlainText(Core()->cmd("pca@x:" + sel_text));
        }
        else    if (parsing == "String")
        {
            ui->hexDisasTextEdit->setPlainText(Core()->cmd("pcs@x:" + sel_text));
        }
        else    if (parsing == "JSON")
        {
            ui->hexDisasTextEdit->setPlainText(Core()->cmd("pcj@x:" + sel_text));
        }
        else    if (parsing == "Javascript")
        {
            ui->hexDisasTextEdit->setPlainText(Core()->cmd("pcJ@x:" + sel_text));
        }
        else    if (parsing == "Python")
        {
            ui->hexDisasTextEdit->setPlainText(Core()->cmd("pcp@x:" + sel_text));
        }

        // Fill the information tab hashes and entropy
        ui->bytesMD5->setText(Core()->cmd("ph md5@x:" + sel_text).trimmed());
        ui->bytesSHA1->setText(Core()->cmd("ph sha1@x:" + sel_text).trimmed());
        ui->bytesEntropy->setText(Core()->cmd("ph entropy@x:" + sel_text).trimmed());
        ui->bytesMD5->setCursorPosition(0);
        ui->bytesSHA1->setCursorPosition(0);
    }
}

void HexdumpWidget::on_hexArchComboBox_2_currentTextChanged(const QString &/*arg1*/)
{
    on_hexHexText_selectionChanged();
}
void HexdumpWidget::on_hexBitsComboBox_2_currentTextChanged(const QString &/*arg1*/)
{
    on_hexHexText_selectionChanged();
}

/*
 * Context menu functions
 */

void HexdumpWidget::showHexdumpContextMenu(const QPoint &pt)
{
    // Set Hexdump popup menu
    QMenu *menu = ui->hexHexText->createStandardContextMenu();
    menu->clear();
    menu->addAction(ui->actionHexCopy_Hexpair);
    menu->addAction(ui->actionHexCopy_ASCII);
    menu->addAction(ui->actionHexCopy_Text);
    menu->addSeparator();
    QMenu *colSubmenu = menu->addMenu("Columns");
    colSubmenu->addAction(ui->action4columns);
    colSubmenu->addAction(ui->action8columns);
    colSubmenu->addAction(ui->action16columns);
    colSubmenu->addAction(ui->action32columns);
    menu->addSeparator();
    menu->addAction(ui->actionHexEdit);
    menu->addAction(ui->actionHexPaste);
    menu->addSeparator();
    menu->addAction(ui->actionHexInsert_Hex);
    menu->addAction(ui->actionHexInsert_String);

    ui->hexHexText->setContextMenuPolicy(Qt::CustomContextMenu);

    menu->exec(ui->hexHexText->mapToGlobal(pt));
    delete menu;
}

void HexdumpWidget::showHexASCIIContextMenu(const QPoint &pt)
{
    // Set Hex ASCII popup menu
    QMenu *menu = ui->hexASCIIText->createStandardContextMenu();
    menu->clear();
    menu->addAction(ui->actionHexCopy_Hexpair);
    menu->addAction(ui->actionHexCopy_ASCII);
    menu->addAction(ui->actionHexCopy_Text);
    menu->addSeparator();
    QMenu *colSubmenu = menu->addMenu("Columns");
    colSubmenu->addAction(ui->action4columns);
    colSubmenu->addAction(ui->action8columns);
    colSubmenu->addAction(ui->action16columns);
    colSubmenu->addAction(ui->action32columns);
    menu->addSeparator();
    menu->addAction(ui->actionHexEdit);
    menu->addAction(ui->actionHexPaste);
    menu->addSeparator();
    menu->addAction(ui->actionHexInsert_Hex);
    menu->addAction(ui->actionHexInsert_String);

    ui->hexASCIIText->setContextMenuPolicy(Qt::CustomContextMenu);

    menu->exec(ui->hexASCIIText->mapToGlobal(pt));
    delete menu;
}


/*
 * Actions callback functions
 */

void HexdumpWidget::on_actionSettings_menu_1_triggered()
{
    bool ok = true;

    QFont font = QFont("Monospace", 8);
    // TODO Use global configuration
    //QFont font = QFontDialog::getFont(&ok, ui->disasTextEdit_2->font(), this);

    if (ok)
    {
        setFonts(font);

        emit fontChanged(font);
    }
}

void HexdumpWidget::setFonts(QFont font)
{
    //ui->disasTextEdit_2->setFont(font);
    // the user clicked OK and font is set to the font the user selected
    //ui->disasTextEdit_2->setFont(font);
    ui->hexOffsetText->setFont(font);
    ui->hexHexText->setFont(font);
    ui->hexASCIIText->setFont(font);
}

void HexdumpWidget::on_actionHideHexdump_side_panel_triggered()
{
    if (ui->hexSideTab_2->isVisible())
    {
        ui->hexSideTab_2->hide();
    }
    else
    {
        ui->hexSideTab_2->show();
    }
}

/*void HexdumpWidget::on_actionSend_to_Notepad_triggered()
{
    QTextCursor cursor = ui->disasTextEdit_2->textCursor();
    QString text = cursor.selectedText();
    // TODO
    // this->main->sendToNotepad(text);
}*/

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

void HexdumpWidget::on_codeCombo_2_currentTextChanged(const QString &arg1)
{
    if (arg1 == "Dissasembly")
    {
        ui->hexSideFrame_2->show();
        ui->hexDisasTextEdit->setPlainText(";; Select some bytes on the left\n;; to see them disassembled");
    }
    else
    {
        ui->hexSideFrame_2->hide();
        ui->hexDisasTextEdit->setPlainText(";; Select some bytes on the left\n;; to see them parsed here");
    }
}

QString HexdumpWidget::normalize_addr(QString addr)
{
    QString base = Core()->cmd("s").split("0x")[1].trimmed();
    int len = base.length();
    if (len < 8)
    {
        int padding = 8 - len;
        QString zero = "0";
        QString zeroes = zero.repeated(padding);
        QString s = "0x" + zeroes + base;
        return s;
    }
    else
    {
        return addr.trimmed();
    }
}

QString HexdumpWidget::normalizeAddr(QString addr)
{
    QString base = addr.split("0x")[1].trimmed();
    int len = base.length();
    if (len < 8)
    {
        int padding = 8 - len;
        QString zero = "0";
        QString zeroes = zero.repeated(padding);
        QString s = "0x" + zeroes + base;
        return s;
    }
    else
    {
        return addr;
    }
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

void HexdumpWidget::on_memSideToolButton_clicked()
{
    if (ui->memSideToolButton->isChecked())
    {
        ui->hexSideTab_2->hide();
        ui->memSideToolButton->setIcon(QIcon(":/img/icons/left_light.svg"));
    }
    else
    {
        ui->hexSideTab_2->show();
        ui->memSideToolButton->setIcon(QIcon(":/img/icons/right_light.svg"));
    }
}

void HexdumpWidget::resizeEvent(QResizeEvent *event)
{
    QDockWidget::resizeEvent(event);

    adjustHexdumpLines();
}

void HexdumpWidget::wheelEvent(QWheelEvent* event)
{
    if( Qt::ControlModifier == event->modifiers() )
    {
        const QPoint numDegrees = event->angleDelta() / 8;
        if(!numDegrees.isNull())
        {
            const QPoint numSteps = numDegrees / 15;
            if( 0 != numSteps.y() )
            {
                if(numSteps.y() > 0)
                {
                    zoomIn(1);
                }
                else if( numSteps.y() < 0 )
                {
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
    if (ui->hexArchComboBox_2->findText(arch) != -1)
    {
        ui->hexArchComboBox_2->setCurrentIndex(ui->hexArchComboBox_2->findText(arch));
    }

    //int bits_index = ui->hexBitsComboBox_2->findText(bits);
    if (ui->hexBitsComboBox_2->findText(bits) != -1)
    {
        ui->hexBitsComboBox_2->setCurrentIndex(ui->hexBitsComboBox_2->findText(bits));
    }
}

void HexdumpWidget::showOffsets(bool show)
{
    if (show)
    {
        ui->hexOffsetText->show();
        Core()->setConfig("asm.offset", 1);
    }
    else
    {
        ui->hexOffsetText->hide();
        Core()->setConfig("asm.offset", 0);
    }
}

void HexdumpWidget::zoomIn(int range)
{
    ui->hexOffsetText->zoomIn(range);
    ui->hexASCIIText->zoomIn(range);
    ui->hexHexText->zoomIn(range);
    resizeHexdump();
}

void HexdumpWidget::zoomOut(int range)
{
    ui->hexOffsetText->zoomOut(range);
    ui->hexASCIIText->zoomOut(range);
    ui->hexHexText->zoomOut(range);
    resizeHexdump();
}
