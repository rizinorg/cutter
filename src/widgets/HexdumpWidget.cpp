
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

static const int blockSize = 16;
static const int blocksMarginMin = 2;
static const int blocksMarginDefault = 3;
static const int blocksMarginMax = 4;

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

    // Synchronize hexdump scrolling
    connect(ui->hexOffsetText->verticalScrollBar(), SIGNAL(valueChanged(int)),
            ui->hexHexText->verticalScrollBar(), SLOT(setValue(int)));
    connect(ui->hexOffsetText->verticalScrollBar(), SIGNAL(valueChanged(int)),
            ui->hexASCIIText->verticalScrollBar(), SLOT(setValue(int)));

    connect(ui->hexHexText->verticalScrollBar(), SIGNAL(valueChanged(int)),
            ui->hexOffsetText->verticalScrollBar(), SLOT(setValue(int)));
    connect(ui->hexHexText->verticalScrollBar(), SIGNAL(valueChanged(int)),
            ui->hexASCIIText->verticalScrollBar(), SLOT(setValue(int)));

    connect(ui->hexASCIIText->verticalScrollBar(), SIGNAL(valueChanged(int)),
            ui->hexOffsetText->verticalScrollBar(), SLOT(setValue(int)));
    connect(ui->hexASCIIText->verticalScrollBar(), SIGNAL(valueChanged(int)),
            ui->hexHexText->verticalScrollBar(), SLOT(setValue(int)));

    // Control Disasm and Hex scroll to add more contents
    connect(ui->hexASCIIText->verticalScrollBar(), SIGNAL(valueChanged(int)), this, SLOT(hexScrolled()));

    connect(Core(), SIGNAL(seekChanged(RVA)), this, SLOT(on_seekChanged(RVA)));
    connect(Core(), SIGNAL(raisePrioritizedMemoryWidget(CutterCore::MemoryWidgetType)), this, SLOT(raisePrioritizedMemoryWidget(CutterCore::MemoryWidgetType)));

    connect(this, &QDockWidget::visibilityChanged, this, [](bool visibility) {
        if (visibility)
        {
            Core()->setMemoryWidgetPriority(CutterCore::MemoryWidgetType::Hexdump);
        }
    });

    fillPlugins();
}

HexdumpWidget::HexdumpWidget(const QString &title, QWidget *parent, Qt::WindowFlags flags)
        : HexdumpWidget(parent, flags)
{
    setWindowTitle(title);
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

    RCoreLocked lcore = Core()->core();
    // Prevent further scroll
    disconnect(ui->hexASCIIText->verticalScrollBar(), SIGNAL(valueChanged(int)), this, SLOT(hexScrolled()));

    // Clear previous content to add new
    ui->hexOffsetText->clear();
    ui->hexHexText->clear();
    ui->hexASCIIText->clear();

    int hexdumpLength;
    int cols = lcore->print->cols;
    ut64 bsize = 128 * cols;
    if (bottomOffset < bsize)
    {
        bottomOffset = 0;
        hexdumpLength = bsize;//-hexdumpBottomOffset;
    }
    else
    {
        hexdumpLength = bsize;
    }

    //int size;
    //size = core->get_size();

    QString s = "";

    // Add first the hexdump at block size --
    QList<QString> ret = this->get_hexdump(RAddressString(addr - hexdumpLength));

    bottomOffset = lcore->offset;
    ui->hexOffsetText->setPlainText(ret[0]);
    ui->hexHexText->setPlainText(ret[1]);
    ui->hexASCIIText->setPlainText(ret[2]);
    resizeHexdump();

    // Get address to move cursor to later
    s = this->normalize_addr(Core()->cmd("s"));
    ret = this->get_hexdump(RAddressString(addr));

    bottomOffset = lcore->offset;
    ui->hexOffsetText->appendPlainText(ret[0]);
    ui->hexHexText->appendPlainText(ret[1]);
    ui->hexASCIIText->appendPlainText(ret[2]);
    resizeHexdump();

    // Move cursor to desired address
    QTextCursor cur = ui->hexOffsetText->textCursor();
    ui->hexOffsetText->ensureCursorVisible();
    ui->hexHexText->ensureCursorVisible();
    ui->hexASCIIText->ensureCursorVisible();
    ui->hexOffsetText->moveCursor(QTextCursor::End);
    ui->hexOffsetText->find(s, QTextDocument::FindBackward);
    ui->hexOffsetText->moveCursor(QTextCursor::EndOfLine, QTextCursor::MoveAnchor);

    connect(ui->hexASCIIText->verticalScrollBar(), SIGNAL(valueChanged(int)), this, SLOT(hexScrolled()));

}

/*
 * Content management functions
 */

void HexdumpWidget::fillPlugins()
{
    // Fill the plugins combo for the hexdump sidebar
    ui->hexArchComboBox_2->insertItems(0, Core()->getAsmPluginNames());
}

QList<QString> HexdumpWidget::get_hexdump(const QString &offset)
{
    RCoreLocked lcore = Core()->core();
    QList<QString> ret;
    QString hexdump;

    int hexdumpLength;
    int cols = lcore->print->cols;
    ut64 bsize = 128 * cols;
    if (bottomOffset < bsize)
    {
        bottomOffset = 0;
        hexdumpLength = bsize;
        //-hexdumpBottomOffset;
    }
    else
    {
        hexdumpLength = bsize;
    }

    //this->main->add_debug_output("BSize: " + Core()->itoa(hexdumpLength, 10));

    if (offset.isEmpty())
    {
        hexdump = Core()->cmd("px " + Core()->itoa(hexdumpLength, 10));
    }
    else
    {
        hexdump = Core()->cmd("px " + Core()->itoa(hexdumpLength, 10) + " @ " + offset);
    }
    //QString hexdump = Core()->cmd ("px 0x" + Core()->itoa(size) + " @ 0x0");
    // TODO: use pxl to simplify
    QString offsets;
    QString hex;
    QString ascii;
    int ln = 0;

    for (const QString line : hexdump.split("\n"))
    {
        if (ln++ == 0)
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
                {
                    hex += a.trimmed() + "\n";
                }
                    break;
                case 2:
                    ascii += a + "\n";
                    break;
            }
        }
    }
    ret << offsets.trimmed();
    ret << hex.trimmed();
    ret << ascii.trimmed();

    return ret;
}

void HexdumpWidget::resizeHexdump()
{
    ui->hexOffsetText->setMinimumWidth(static_cast<int>(ui->hexOffsetText->document()->size().width()));
    ui->hexHexText->setMinimumWidth(static_cast<int>(ui->hexHexText->document()->size().width()));
    //this->hexASCIIText->setMinimumWidth(this->hexASCIIText->document()->size().width());
}

void HexdumpWidget::hexScrolled()
{
    RCoreLocked lcore = Core()->core();
    QScrollBar *sb = ui->hexASCIIText->verticalScrollBar();

    if (sb->value() > sb->maximum() - 10)
    {
        //this->main->addDebugOutput("End is coming");

        QTextCursor tc = ui->hexOffsetText->textCursor();
        tc.movePosition(QTextCursor::End);
        tc.select(QTextCursor::LineUnderCursor);
        QString lastline = tc.selectedText();
        //this->main->add_debug_output("Last Offset/VA: " + lastline);
        //refreshHexdump(2);

        QList<QString> ret = this->get_hexdump(lastline);

        // To prevent recursive calls to hexScrolled (this function) blocks the
        // scroll bar signals
        auto appendTextWithoutSignals = [](QPlainTextEdit *edit, const QString & text)
        {
            edit->verticalScrollBar()->blockSignals(true);
            edit->appendPlainText(text);
            edit->verticalScrollBar()->blockSignals(false);
        };

        appendTextWithoutSignals(ui->hexOffsetText, ret[0]);
        appendTextWithoutSignals(ui->hexHexText, ret[1]);
        appendTextWithoutSignals(ui->hexASCIIText, ret[2]);
        resizeHexdump();

        // Append more hex text here
        //   ui->disasTextEdit->moveCursor(QTextCursor::Start);
        // ui->disasTextEdit->insertPlainText(core->cmd("pd@$$-100"));
        //... same for the other text (offset and hex text edits)
    }
    else if (sb->value() < sb->minimum() + 10)
    {
        //this->main->add_debug_output("Begining is coming");

        QTextCursor tc = ui->hexOffsetText->textCursor();
        tc.movePosition(QTextCursor::Start);
        tc.select(QTextCursor::LineUnderCursor);
        QString firstline = tc.selectedText();
        //disathis->main->add_debug_output("First Offset/VA: " + firstline);
        //refreshHexdump(1);

        //int cols = lcore->print->cols;
        // px bsize @ addr
        //int bsize = 128 * cols;
        int bsize = 800;
        QString s = QString::number(bsize);
        // s = 2048.. sigh...
        QString kk = Core()->cmd("? " + firstline + " - " + s);
        QString k = kk.split(" ")[1];

        QList<QString> ret = this->get_hexdump(k);

        // Prevent further scroll
        disconnect(ui->hexASCIIText->verticalScrollBar(), SIGNAL(valueChanged(int)), this, SLOT(hexScrolled()));
        // Get actual maximum scrolling value
        int b = ui->hexASCIIText->verticalScrollBar()->maximum();

        // Add new offset content
        QTextDocument *offset_document = ui->hexOffsetText->document();
        QTextCursor offset_cursor(offset_document);
        offset_cursor.movePosition(QTextCursor::Start, QTextCursor::MoveAnchor);
        offset_cursor.insertText(ret[0] + "\n");

        // Add new hex content
        QTextDocument *hex_document = ui->hexHexText->document();
        QTextCursor hex_cursor(hex_document);
        hex_cursor.movePosition(QTextCursor::Start, QTextCursor::MoveAnchor);
        hex_cursor.insertText(ret[1] + "\n");

        // Add new ASCII content
        QTextDocument *ascii_document = ui->hexASCIIText->document();
        QTextCursor ascii_cursor(ascii_document);
        ascii_cursor.movePosition(QTextCursor::Start, QTextCursor::MoveAnchor);
        ascii_cursor.insertText(ret[2] + "\n");

        // Get new maximum scroll value
        int c = ui->hexASCIIText->verticalScrollBar()->maximum();
        // Get size of new added content
        int z = c - b;
        // Get new slider position
        int a = ui->hexASCIIText->verticalScrollBar()->sliderPosition();
        // move to previous position
        ui->hexASCIIText->verticalScrollBar()->setValue(a + z);

        this->resizeHexdump();
        connect(ui->hexASCIIText->verticalScrollBar(), SIGNAL(valueChanged(int)), this, SLOT(hexScrolled()));
    }
}

void HexdumpWidget::on_hexHexText_2_selectionChanged()
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
    on_hexHexText_2_selectionChanged();
}

void HexdumpWidget::on_hexBitsComboBox_2_currentTextChanged(const QString &/*arg1*/)
{
    on_hexHexText_2_selectionChanged();
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
    // FIXME
    /*
    if (main->responsive && isVisible())
    {
        if (event->size().width() <= 1150)
        {
            ui->frame_3->setVisible(false);
            ui->memPreviewTab->setVisible(false);
            ui->previewToolButton_2->setChecked(false);
            if (event->size().width() <= 950)
            {
                ui->memSideTabWidget_2->hide();
                ui->hexSideTab_2->hide();
                ui->memSideToolButton->setChecked(true);
            }
            else
            {
                ui->memSideTabWidget_2->show();
                ui->hexSideTab_2->show();
                ui->memSideToolButton->setChecked(false);
            }
        }
        else
        {
            ui->frame_3->setVisible(true);
            ui->memPreviewTab->setVisible(true);
            ui->previewToolButton_2->setChecked(true);
        }
    }
    */
    QDockWidget::resizeEvent(event);
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
