
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

HexdumpWidget::HexdumpWidget(QWidget *parent, Qt::WindowFlags flags) :
        QDockWidget(parent, flags),
        ui(new Ui::HexdumpWidget),
        core(CutterCore::getInstance())
{
    ui->setupUi(this);
    this->hexOffsetText = ui->hexOffsetText_2;
    this->hexHexText = ui->hexHexText_2;
    this->hexASCIIText = ui->hexASCIIText_2;

    this->hexDisasTextEdit = ui->hexDisasTextEdit_2;

    //this->on_actionSettings_menu_1_triggered();

    // Setup hex highlight
    //connect(ui->hexHexText, SIGNAL(cursorPositionChanged()), this, SLOT(highlightHexCurrentLine()));
    //highlightHexCurrentLine();

    // Normalize fonts for other OS
    qhelpers::normalizeEditFont(this->hexOffsetText);
    qhelpers::normalizeEditFont(this->hexHexText);
    qhelpers::normalizeEditFont(this->hexASCIIText);

    // Popup menu on Settings toolbutton
    QMenu *memMenu = new QMenu();
    ui->memSettingsButton_2->addAction(ui->actionSettings_menu_1);
    memMenu->addAction(ui->actionSettings_menu_1);
    ui->memSettingsButton_2->setMenu(memMenu);

    // Set hexdump context menu
    ui->hexHexText_2->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(ui->hexHexText_2, SIGNAL(customContextMenuRequested(const QPoint &)),
            this, SLOT(showHexdumpContextMenu(const QPoint &)));
    ui->hexASCIIText_2->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(ui->hexASCIIText_2, SIGNAL(customContextMenuRequested(const QPoint &)),
            this, SLOT(showHexASCIIContextMenu(const QPoint &)));

    // Syncronize hexdump scrolling
    connect(ui->hexOffsetText_2->verticalScrollBar(), SIGNAL(valueChanged(int)),
            ui->hexHexText_2->verticalScrollBar(), SLOT(setValue(int)));
    connect(ui->hexOffsetText_2->verticalScrollBar(), SIGNAL(valueChanged(int)),
            ui->hexASCIIText_2->verticalScrollBar(), SLOT(setValue(int)));

    connect(ui->hexHexText_2->verticalScrollBar(), SIGNAL(valueChanged(int)),
            ui->hexOffsetText_2->verticalScrollBar(), SLOT(setValue(int)));
    connect(ui->hexHexText_2->verticalScrollBar(), SIGNAL(valueChanged(int)),
            ui->hexASCIIText_2->verticalScrollBar(), SLOT(setValue(int)));

    connect(ui->hexASCIIText_2->verticalScrollBar(), SIGNAL(valueChanged(int)),
            ui->hexOffsetText_2->verticalScrollBar(), SLOT(setValue(int)));
    connect(ui->hexASCIIText_2->verticalScrollBar(), SIGNAL(valueChanged(int)),
            ui->hexHexText_2->verticalScrollBar(), SLOT(setValue(int)));

    // Control Disasm and Hex scroll to add more contents
    connect(this->hexASCIIText->verticalScrollBar(), SIGNAL(valueChanged(int)), this, SLOT(hexScrolled()));

    connect(core, SIGNAL(seekChanged(RVA)), this, SLOT(on_seekChanged(RVA)));

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

HexdumpWidget::~HexdumpWidget() {}

/*
 * Text highlight functions
 */
void HexdumpWidget::highlightHexCurrentLine()
{
    QList<QTextEdit::ExtraSelection> extraSelections;

    if (!ui->hexHexText_2->isReadOnly())
    {
        QTextEdit::ExtraSelection selection;

        QColor lineColor = QColor(190, 144, 212);

        selection.format.setBackground(lineColor);
        selection.format.setProperty(QTextFormat::FullWidthSelection, true);
        selection.cursor = ui->hexHexText_2->textCursor();
        selection.cursor.clearSelection();
        extraSelections.append(selection);
    }

    QTextCursor cursor = ui->hexHexText_2->textCursor();
    cursor.select(QTextCursor::WordUnderCursor);

    QTextEdit::ExtraSelection currentWord;

    QColor blueColor = QColor(Qt::blue).lighter(160);
    currentWord.format.setBackground(blueColor);

    currentWord.cursor = cursor;
    extraSelections.append(currentWord);

    ui->hexHexText_2->setExtraSelections(extraSelections);

    highlightHexWords(cursor.selectedText());
}

void HexdumpWidget::highlightHexWords(const QString &str)
{
    QString searchString = str;
    QTextDocument *document = ui->hexHexText_2->document();

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
        addr = core->getSeekAddr();
    }

    RCoreLocked lcore = this->core->core();
    // Prevent further scroll
    disconnect(this->hexASCIIText->verticalScrollBar(), SIGNAL(valueChanged(int)), this, SLOT(hexScrolled()));

    // Clear previous content to add new
    this->hexOffsetText->clear();
    this->hexHexText->clear();
    this->hexASCIIText->clear();

    int hexdumpLength;
    int cols = lcore->print->cols;
    ut64 bsize = 128 * cols;
    if (hexdumpBottomOffset < bsize)
    {
        hexdumpBottomOffset = 0;
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

    hexdumpBottomOffset = lcore->offset;
    this->hexOffsetText->setPlainText(ret[0]);
    this->hexHexText->setPlainText(ret[1]);
    this->hexASCIIText->setPlainText(ret[2]);
    this->resizeHexdump();

    // Get address to move cursor to later
    s = this->normalize_addr(this->core->cmd("s"));
    ret = this->get_hexdump(RAddressString(addr));

    hexdumpBottomOffset = lcore->offset;
    this->hexOffsetText->append(ret[0]);
    this->hexHexText->append(ret[1]);
    this->hexASCIIText->append(ret[2]);
    this->resizeHexdump();

    // Move cursor to desired address
    QTextCursor cur = this->hexOffsetText->textCursor();
    this->hexOffsetText->ensureCursorVisible();
    this->hexHexText->ensureCursorVisible();
    this->hexASCIIText->ensureCursorVisible();
    this->hexOffsetText->moveCursor(QTextCursor::End);
    this->hexOffsetText->find(s, QTextDocument::FindBackward);
    this->hexOffsetText->moveCursor(QTextCursor::EndOfLine, QTextCursor::MoveAnchor);

    connect(this->hexASCIIText->verticalScrollBar(), SIGNAL(valueChanged(int)), this, SLOT(hexScrolled()));

}

/*
 * Content management functions
 */

void HexdumpWidget::fillPlugins()
{
    // Fill the plugins combo for the hexdump sidebar
    ui->hexArchComboBox_2->insertItems(0, core->getAsmPluginNames());
}

QList<QString> HexdumpWidget::get_hexdump(const QString &offset)
{
    RCoreLocked lcore = this->core->core();
    QList<QString> ret;
    QString hexdump;

    int hexdumpLength;
    int cols = lcore->print->cols;
    ut64 bsize = 128 * cols;
    if (hexdumpBottomOffset < bsize)
    {
        hexdumpBottomOffset = 0;
        hexdumpLength = bsize;
        //-hexdumpBottomOffset;
    }
    else
    {
        hexdumpLength = bsize;
    }

    //this->main->add_debug_output("BSize: " + this->core->itoa(hexdumpLength, 10));

    if (offset.isEmpty())
    {
        hexdump = this->core->cmd("px " + this->core->itoa(hexdumpLength, 10));
    }
    else
    {
        hexdump = this->core->cmd("px " + this->core->itoa(hexdumpLength, 10) + " @ " + offset);
    }
    //QString hexdump = this->core->cmd ("px 0x" + this->core->itoa(size) + " @ 0x0");
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
    this->hexOffsetText->setMinimumWidth(this->hexOffsetText->document()->size().width());
    this->hexHexText->setMinimumWidth(this->hexHexText->document()->size().width());
    this->hexASCIIText->setMinimumWidth(this->hexASCIIText->document()->size().width());
}

void HexdumpWidget::hexScrolled()
{
    RCoreLocked lcore = this->core->core();
    QScrollBar *sb = this->hexASCIIText->verticalScrollBar();

    if (sb->value() > sb->maximum() - 10)
    {
        //this->main->addDebugOutput("End is coming");

        QTextCursor tc = this->hexOffsetText->textCursor();
        tc.movePosition(QTextCursor::End);
        tc.select(QTextCursor::LineUnderCursor);
        QString lastline = tc.selectedText();
        //this->main->add_debug_output("Last Offset/VA: " + lastline);
        //refreshHexdump(2);

        QList<QString> ret = this->get_hexdump(lastline);

        // To prevent recursive calls to hexScrolled (this function) blocks the
        // scroll bar signals
        auto appendTextWithoutSignals = [](QTextEdit * edit, const QString & text)
        {
            edit->verticalScrollBar()->blockSignals(true);
            edit->append(text);
            edit->verticalScrollBar()->blockSignals(false);
        };

        appendTextWithoutSignals(hexOffsetText, ret[0]);
        appendTextWithoutSignals(hexHexText, ret[1]);
        appendTextWithoutSignals(hexASCIIText, ret[2]);
        this->resizeHexdump();

        // Append more hex text here
        //   ui->disasTextEdit->moveCursor(QTextCursor::Start);
        // ui->disasTextEdit->insertPlainText(core->cmd("pd@$$-100"));
        //... same for the other text (offset and hex text edits)
    }
    else if (sb->value() < sb->minimum() + 10)
    {
        //this->main->add_debug_output("Begining is coming");

        QTextCursor tc = this->hexOffsetText->textCursor();
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
        QString kk = this->core->cmd("? " + firstline + " - " + s);
        QString k = kk.split(" ")[1];

        QList<QString> ret = this->get_hexdump(k);

        // Prevent further scroll
        disconnect(this->hexASCIIText->verticalScrollBar(), SIGNAL(valueChanged(int)), this, SLOT(hexScrolled()));
        // Get actual maximum scrolling value
        int b = this->hexASCIIText->verticalScrollBar()->maximum();

        // Add new offset content
        QTextDocument *offset_document = this->hexOffsetText->document();
        QTextCursor offset_cursor(offset_document);
        offset_cursor.movePosition(QTextCursor::Start, QTextCursor::MoveAnchor);
        offset_cursor.insertText(ret[0] + "\n");

        // Add new hex content
        QTextDocument *hex_document = this->hexHexText->document();
        QTextCursor hex_cursor(hex_document);
        hex_cursor.movePosition(QTextCursor::Start, QTextCursor::MoveAnchor);
        hex_cursor.insertText(ret[1] + "\n");

        // Add new ASCII content
        QTextDocument *ascii_document = this->hexASCIIText->document();
        QTextCursor ascii_cursor(ascii_document);
        ascii_cursor.movePosition(QTextCursor::Start, QTextCursor::MoveAnchor);
        ascii_cursor.insertText(ret[2] + "\n");

        // Get new maximum scroll value
        int c = this->hexASCIIText->verticalScrollBar()->maximum();
        // Get size of new added content
        int z = c - b;
        // Get new slider position
        int a = this->hexASCIIText->verticalScrollBar()->sliderPosition();
        // move to previous position
        this->hexASCIIText->verticalScrollBar()->setValue(a + z);

        this->resizeHexdump();
        connect(this->hexASCIIText->verticalScrollBar(), SIGNAL(valueChanged(int)), this, SLOT(hexScrolled()));
    }
}

void HexdumpWidget::on_hexHexText_2_selectionChanged()
{
    // Get selected partsing type
    QString parsing = ui->codeCombo_2->currentText();
    // Get selected text
    QTextCursor cursor(this->hexHexText->textCursor());
    QString sel_text = cursor.selectedText();

    sel_text = sel_text.simplified().remove(" ");
    //eprintf ("-- (((%s))) --\n", sel_text.toUtf8().constData());

    if (sel_text == "")
    {
        this->hexDisasTextEdit->setPlainText("");
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

            QString oarch = this->core->getConfig("asm.arch");
            QString obits = this->core->getConfig("asm.bits");

            this->core->setConfig("asm.arch", arch);
            this->core->setConfig("asm.bits", bits);
            QString str = this->core->cmd("pad " + sel_text);
            this->hexDisasTextEdit->setPlainText(str);
            this->core->setConfig("asm.arch", oarch);
            this->core->setConfig("asm.bits", obits);
            //qDebug() << "Selected Arch: " << arch;
            //qDebug() << "Selected Bits: " << bits;
            //qDebug() << "Selected Text: " << sel_text;
        }
            // TODO: update on selection changes.. use cmd("pc "+len+"@"+off)
        else if (parsing == "C byte array")
        {
            this->hexDisasTextEdit->setPlainText(this->core->cmd("pc@x:" + sel_text));
        }
        else    if (parsing == "C dword array")
        {
            this->hexDisasTextEdit->setPlainText(this->core->cmd("pcw@x:" + sel_text));
        }
        else    if (parsing == "C qword array")
        {
            this->hexDisasTextEdit->setPlainText(this->core->cmd("pcq@x:" + sel_text));
        }
        else    if (parsing == "Assembler")
        {
            this->hexDisasTextEdit->setPlainText(this->core->cmd("pca@x:" + sel_text));
        }
        else    if (parsing == "String")
        {
            this->hexDisasTextEdit->setPlainText(this->core->cmd("pcs@x:" + sel_text));
        }
        else    if (parsing == "JSON")
        {
            this->hexDisasTextEdit->setPlainText(this->core->cmd("pcj@x:" + sel_text));
        }
        else    if (parsing == "Javascript")
        {
            this->hexDisasTextEdit->setPlainText(this->core->cmd("pcJ@x:" + sel_text));
        }
        else    if (parsing == "Python")
        {
            this->hexDisasTextEdit->setPlainText(this->core->cmd("pcp@x:" + sel_text));
        }

        // Fill the information tab hashes and entropy
        ui->bytesMD5->setText(this->core->cmd("ph md5@x:" + sel_text).trimmed());
        ui->bytesSHA1->setText(this->core->cmd("ph sha1@x:" + sel_text).trimmed());
        ui->bytesEntropy->setText(this->core->cmd("ph entropy@x:" + sel_text).trimmed());
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
    QMenu *menu = ui->hexHexText_2->createStandardContextMenu();
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

    ui->hexHexText_2->setContextMenuPolicy(Qt::CustomContextMenu);

    menu->exec(ui->hexHexText_2->mapToGlobal(pt));
    delete menu;
}

void HexdumpWidget::showHexASCIIContextMenu(const QPoint &pt)
{
    // Set Hex ASCII popup menu
    QMenu *menu = ui->hexASCIIText_2->createStandardContextMenu();
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

    ui->hexASCIIText_2->setContextMenuPolicy(Qt::CustomContextMenu);

    menu->exec(ui->hexASCIIText_2->mapToGlobal(pt));
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
    ui->hexOffsetText_2->setFont(font);
    ui->hexHexText_2->setFont(font);
    ui->hexASCIIText_2->setFont(font);
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
    this->core->setConfig("hex.cols", 8);
    this->refresh();
}

void HexdumpWidget::on_action16columns_triggered()
{
    this->core->setConfig("hex.cols", 16);
    this->refresh();
}

void HexdumpWidget::on_action4columns_triggered()
{
    this->core->setConfig("hex.cols", 4);
    this->refresh();
}

void HexdumpWidget::on_action32columns_triggered()
{
    this->core->setConfig("hex.cols", 32);
    this->refresh();
}

void HexdumpWidget::on_action64columns_triggered()
{
    this->core->setConfig("hex.cols", 64);
    this->refresh();
}

void HexdumpWidget::on_action2columns_triggered()
{
    this->core->setConfig("hex.cols", 2);
    this->refresh();
}

void HexdumpWidget::on_action1column_triggered()
{
    this->core->setConfig("hex.cols", 1);
    this->refresh();
}

void HexdumpWidget::on_codeCombo_2_currentTextChanged(const QString &arg1)
{
    if (arg1 == "Dissasembly")
    {
        ui->hexSideFrame_2->show();
        ui->hexDisasTextEdit_2->setPlainText(";; Select some bytes on the left\n;; to see them disassembled");
    }
    else
    {
        ui->hexSideFrame_2->hide();
        ui->hexDisasTextEdit_2->setPlainText(";; Select some bytes on the left\n;; to see them parsed here");
    }
}

QString HexdumpWidget::normalize_addr(QString addr)
{
    QString base = this->core->cmd("s").split("0x")[1].trimmed();
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
    QString arch = this->core->cmd("e asm.arch").trimmed();
    QString bits = this->core->cmd("e asm.bits").trimmed();

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
        this->hexOffsetText->show();
        core->setConfig("asm.offset", 1);
    }
    else
    {
        this->hexOffsetText->hide();
        core->setConfig("asm.offset", 0);
    }
}
