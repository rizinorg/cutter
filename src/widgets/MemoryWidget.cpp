#include "MemoryWidget.h"
#include "ui_MemoryWidget.h"
#include "DisassemblerGraphView.h"

#include "MainWindow.h"
#include "utils/Helpers.h"

#include <QTemporaryFile>
#include <QFontDialog>
#include <QScrollBar>
#include <QClipboard>
#include <QShortcut>
#include <QWebEnginePage>
#include <QMenu>
#include <QFont>
#include <QUrl>
#include <QWebEngineSettings>
#include <QWebEngineProfile>
#include <QSettings>

#include <cassert>

MemoryWidget::MemoryWidget() :
    ui(new Ui::MemoryWidget),
    core(CutterCore::getInstance())
{
    ui->setupUi(this);
    this->hexOffsetText = ui->hexOffsetText_2;
    this->hexHexText = ui->hexHexText_2;
    this->hexDisasTextEdit = ui->hexDisasTextEdit_2;
    this->hexASCIIText = ui->hexASCIIText_2;
    this->xrefToTreeWidget_2 = ui->xrefToTreeWidget_2;
    this->xreFromTreeWidget_2 = ui->xreFromTreeWidget_2;
    this->memTabWidget = ui->memTabWidget;

    this->last_fcn = "entry0";
    this->last_graph_fcn = 0; //"";
    this->last_hexdump_fcn = 0; //"";

    disasm_top_offset = 0;
    next_disasm_top_offset = 0;

    //this->on_actionSettings_menu_1_triggered();

    // Setup hex highlight
    //connect(ui->hexHexText, SIGNAL(cursorPositionChanged()), this, SLOT(highlightHexCurrentLine()));
    //highlightHexCurrentLine();

    // Highlight current line on previews and decompiler
    connect(ui->previewTextEdit, SIGNAL(cursorPositionChanged()), this, SLOT(highlightPreviewCurrentLine()));
    connect(ui->decoTextEdit, SIGNAL(cursorPositionChanged()), this, SLOT(highlightDecoCurrentLine()));

    // Hide memview notebooks tabs
    QTabBar *bar = ui->memTabWidget->tabBar();
    bar->setVisible(false);
    QTabBar *sidebar = ui->memSideTabWidget_2->tabBar();
    sidebar->setVisible(false);
    QTabBar *preTab = ui->memPreviewTab->tabBar();
    preTab->setVisible(false);

    // Hide fcn graph notebooks tabs
    QTabBar *graph_bar = ui->fcnGraphTabWidget->tabBar();
    graph_bar->setVisible(false);

    // Debug console
    // For QWebEngine debugging see: https://doc.qt.io/qt-5/qtwebengine-debugging.html
    //QWebSettings::globalSettings()->setAttribute(QWebSettings::DeveloperExtrasEnabled, true);

    // Add margin to function name line edit
    ui->fcnNameEdit->setTextMargins(5, 0, 0, 0);

    // Normalize fonts for other OS
    qhelpers::normalizeEditFont(this->hexOffsetText);
    qhelpers::normalizeEditFont(this->hexHexText);
    qhelpers::normalizeEditFont(this->hexASCIIText);

    // Popup menu on Settings toolbutton
    QMenu *memMenu = new QMenu();
    ui->memSettingsButton_2->addAction(ui->actionSettings_menu_1);
    memMenu->addAction(ui->actionSettings_menu_1);
    ui->memSettingsButton_2->setMenu(memMenu);

    // Set Splitter stretch factor
    ui->splitter->setStretchFactor(0, 10);
    ui->splitter->setStretchFactor(1, 1);

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

    // Space to switch between disassembly and graph
    QShortcut *graph_shortcut = new QShortcut(QKeySequence(Qt::Key_Space), this);
    connect(graph_shortcut, SIGNAL(activated()), this, SLOT(cycleViews()));
    //graph_shortcut->setContext(Qt::WidgetShortcut);

    // Control Disasm and Hex scroll to add more contents
    connect(this->hexASCIIText->verticalScrollBar(), SIGNAL(valueChanged(int)), this, SLOT(hexScrolled()));

    connect(core, SIGNAL(seekChanged(RVA)), this, SLOT(on_seekChanged(RVA)));
    //connect(main, SIGNAL(cursorAddressChanged(RVA)), this, SLOT(on_cursorAddressChanged(RVA)));
    connect(core, SIGNAL(flagsChanged()), this, SLOT(updateViews()));
    connect(core, SIGNAL(commentsChanged()), this, SLOT(updateViews()));
    connect(core, SIGNAL(asmOptionsChanged()), this, SLOT(updateViews()));

    fillPlugins();
}


void MemoryWidget::on_seekChanged(RVA addr)
{
    updateViews(addr);
}

void MemoryWidget::on_cursorAddressChanged(RVA addr)
{
    setFcnName(addr);
    get_refs_data(addr);
}

/*
 * Text highlight functions
 */
void MemoryWidget::highlightHexCurrentLine()
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

void MemoryWidget::highlightHexWords(const QString &str)
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

void MemoryWidget::highlightPreviewCurrentLine()
{

    QList<QTextEdit::ExtraSelection> extraSelections;

    if (ui->previewTextEdit->toPlainText() != "")
    {
        if (ui->previewTextEdit->isReadOnly())
        {
            QTextEdit::ExtraSelection selection;

            QColor lineColor = QColor(190, 144, 212);

            selection.format.setBackground(lineColor);
            selection.format.setProperty(QTextFormat::FullWidthSelection, true);
            selection.cursor = ui->previewTextEdit->textCursor();
            selection.cursor.clearSelection();
            extraSelections.append(selection);
        }
    }
    ui->previewTextEdit->setExtraSelections(extraSelections);
}

void MemoryWidget::highlightDecoCurrentLine()
{

    QList<QTextEdit::ExtraSelection> extraSelections;

    if (ui->decoTextEdit->toPlainText() != "")
    {
        if (ui->decoTextEdit->isReadOnly())
        {
            QTextEdit::ExtraSelection selection;

            QColor lineColor = QColor(190, 144, 212);

            selection.format.setBackground(lineColor);
            selection.format.setProperty(QTextFormat::FullWidthSelection, true);
            selection.cursor = ui->decoTextEdit->textCursor();
            selection.cursor.clearSelection();
            extraSelections.append(selection);
        }
    }
    ui->decoTextEdit->setExtraSelections(extraSelections);
}

MemoryWidget::~MemoryWidget() {}

void MemoryWidget::setup()
{
    setScrollMode();

    const QString off = core->cmd("afo entry0").trimmed();
    RVA offset = off.toULongLong(0, 16);
    updateViews(offset);

    //refreshDisasm();
    //refreshHexdump(off);
    //create_graph(off);
    get_refs_data(offset);
    //setFcnName(off);
}

void MemoryWidget::refresh()
{
    setScrollMode();

    // TODO: honor the offset
    updateViews(RVA_INVALID);
}

/*
 * Content management functions
 */

void MemoryWidget::fillPlugins()
{
    // Fill the plugins combo for the hexdump sidebar
    ui->hexArchComboBox_2->insertItems(0, core->getAsmPluginNames());
}

void MemoryWidget::refreshHexdump(const QString &where)
{
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

    if (!where.isEmpty())
    {
        this->core->cmd("ss " + where);
    }

    // Add first the hexdump at block size --
    this->core->cmd("ss-" + this->core->itoa(hexdumpLength));
    //s = this->normalize_addr(this->core->cmd("s"));
    QList<QString> ret = this->get_hexdump("");

    hexdumpBottomOffset = lcore->offset;
    this->hexOffsetText->setPlainText(ret[0]);
    this->hexHexText->setPlainText(ret[1]);
    this->hexASCIIText->setPlainText(ret[2]);
    this->resizeHexdump();

    // Add then the hexdump at block size ++
    this->core->cmd("ss+" + this->core->itoa(hexdumpLength));
    // Get address to move cursor to later
    //QString s = "0x0" + this->core->cmd("s").split("0x")[1].trimmed();
    s = this->normalize_addr(this->core->cmd("s"));
    ret = this->get_hexdump("");

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

QList<QString> MemoryWidget::get_hexdump(const QString &offset)
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

void MemoryWidget::resizeHexdump()
{
    this->hexOffsetText->setMinimumWidth(this->hexOffsetText->document()->size().width());
    this->hexHexText->setMinimumWidth(this->hexHexText->document()->size().width());
    this->hexASCIIText->setMinimumWidth(this->hexASCIIText->document()->size().width());
}

void MemoryWidget::hexScrolled()
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

void MemoryWidget::on_hexHexText_2_selectionChanged()
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

void MemoryWidget::on_hexArchComboBox_2_currentTextChanged(const QString &/*arg1*/)
{
    on_hexHexText_2_selectionChanged();
}

void MemoryWidget::on_hexBitsComboBox_2_currentTextChanged(const QString &/*arg1*/)
{
    on_hexHexText_2_selectionChanged();
}

/*
 * Context menu functions
 */

void MemoryWidget::showHexdumpContextMenu(const QPoint &pt)
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

void MemoryWidget::showHexASCIIContextMenu(const QPoint &pt)
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

void MemoryWidget::on_showInfoButton_2_clicked()
{
    if (ui->showInfoButton_2->isChecked())
    {
        ui->fcnGraphTabWidget->hide();
        ui->showInfoButton_2->setArrowType(Qt::RightArrow);
    }
    else
    {
        ui->fcnGraphTabWidget->show();
        ui->showInfoButton_2->setArrowType(Qt::DownArrow);
    }
}

void MemoryWidget::on_offsetToolButton_clicked()
{
    if (ui->offsetToolButton->isChecked())
    {
        ui->offsetTreeWidget->hide();
        ui->offsetToolButton->setArrowType(Qt::RightArrow);
    }
    else
    {
        ui->offsetTreeWidget->show();
        ui->offsetToolButton->setArrowType(Qt::DownArrow);
    }
}

/*
 * Show widgets
 */

void MemoryWidget::cycleViews()
{
    switch (ui->memTabWidget->currentIndex())
    {
    case 0:
        // Show hexdump
        ui->hexButton->setChecked(true);
        on_hexButton_clicked();
        break;
    case 1:
        // Show disasm
        ui->disasButton->setChecked(true);
        on_disasButton_clicked();
        break;
    }
}

/*
 * Actions callback functions
 */

void MemoryWidget::on_actionSettings_menu_1_triggered()
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

void MemoryWidget::setFonts(QFont font)
{
    //ui->disasTextEdit_2->setFont(font);
    // the user clicked OK and font is set to the font the user selected
    //ui->disasTextEdit_2->setFont(font);
    ui->hexOffsetText_2->setFont(font);
    ui->hexHexText_2->setFont(font);
    ui->hexASCIIText_2->setFont(font);
    ui->previewTextEdit->setFont(font);
    ui->decoTextEdit->setFont(font);
}

void MemoryWidget::on_actionHideDisasm_side_panel_triggered()
{
    if (ui->memSideTabWidget_2->isVisible())
    {
        ui->memSideTabWidget_2->hide();
    }
    else
    {
        ui->memSideTabWidget_2->show();
    }
}

void MemoryWidget::on_actionHideHexdump_side_panel_triggered()
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

void MemoryWidget::on_actionHideGraph_side_panel_triggered()
{
    if (ui->graphTreeWidget_2->isVisible())
    {
        ui->graphTreeWidget_2->hide();
    }
    else
    {
        ui->graphTreeWidget_2->show();
    }
}

/*
 * Buttons callback functions
 */

void MemoryWidget::on_disasButton_clicked()
{
    ui->memTabWidget->setCurrentIndex(0);
    ui->memSideTabWidget_2->setCurrentIndex(0);
}

void MemoryWidget::on_hexButton_clicked()
{
    ui->memTabWidget->setCurrentIndex(1);
    ui->memSideTabWidget_2->setCurrentIndex(1);
}

/*void MemoryWidget::on_actionSend_to_Notepad_triggered()
{
    QTextCursor cursor = ui->disasTextEdit_2->textCursor();
    QString text = cursor.selectedText();
    // TODO
    // this->main->sendToNotepad(text);
}*/

void MemoryWidget::on_action8columns_triggered()
{
    this->core->setConfig("hex.cols", 8);
    this->refreshHexdump();
}

void MemoryWidget::on_action16columns_triggered()
{
    this->core->setConfig("hex.cols", 16);
    this->refreshHexdump();
}

void MemoryWidget::on_action4columns_triggered()
{
    this->core->setConfig("hex.cols", 4);
    this->refreshHexdump();
}

void MemoryWidget::on_action32columns_triggered()
{
    this->core->setConfig("hex.cols", 32);
    this->refreshHexdump();
}

void MemoryWidget::on_action64columns_triggered()
{
    this->core->setConfig("hex.cols", 64);
    this->refreshHexdump();
}

void MemoryWidget::on_action2columns_triggered()
{
    this->core->setConfig("hex.cols", 2);
    this->refreshHexdump();
}

void MemoryWidget::on_action1column_triggered()
{
    this->core->setConfig("hex.cols", 1);
    this->refreshHexdump();
}

void MemoryWidget::on_xreFromTreeWidget_2_itemDoubleClicked(QTreeWidgetItem *item, int /*column*/)
{
    XrefDescription xref = item->data(0, Qt::UserRole).value<XrefDescription>();
    this->core->seek(xref.to);
}

void MemoryWidget::on_xrefToTreeWidget_2_itemDoubleClicked(QTreeWidgetItem *item, int /*column*/)
{
    XrefDescription xref = item->data(0, Qt::UserRole).value<XrefDescription>();
    this->core->seek(xref.from);
}

void MemoryWidget::on_xrefFromToolButton_2_clicked()
{
    if (ui->xrefFromToolButton_2->isChecked())
    {
        ui->xreFromTreeWidget_2->hide();
        ui->xrefFromToolButton_2->setArrowType(Qt::RightArrow);
    }
    else
    {
        ui->xreFromTreeWidget_2->show();
        ui->xrefFromToolButton_2->setArrowType(Qt::DownArrow);
    }
}

void MemoryWidget::on_xrefToToolButton_2_clicked()
{
    if (ui->xrefToToolButton_2->isChecked())
    {
        ui->xrefToTreeWidget_2->hide();
        ui->xrefToToolButton_2->setArrowType(Qt::RightArrow);
    }
    else
    {
        ui->xrefToTreeWidget_2->show();
        ui->xrefToToolButton_2->setArrowType(Qt::DownArrow);
    }
}

void MemoryWidget::on_codeCombo_2_currentTextChanged(const QString &arg1)
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

void MemoryWidget::get_refs_data(RVA addr)
{
    // refs = calls q hace esa funcion
    QList<XrefDescription> refs = core->getXRefs(addr, false, false);

    // xrefs = calls a esa funcion
    QList<XrefDescription> xrefs = core->getXRefs(addr, true, false);

    // Data for the disasm side graph
    QList<int> data;
    //qDebug() << "Refs:" << refs.size();
    data << refs.size();
    //qDebug() << "XRefs:" << xrefs.size();
    data << xrefs.size();
    //qDebug() << "CC: " << this->core->fcnCyclomaticComplexity(offset.toLong(&ok, 16));
    //data << this->core->fcnCyclomaticComplexity(offset.toLong(&ok, 16));
    data << this->core->getCycloComplex(addr);
    //qDebug() << "BB: " << this->core->fcnBasicBlockCount(offset.toLong(&ok, 16));
    data << this->core->fcnBasicBlockCount(addr);
    data << this->core->fcnEndBbs(addr);
    //qDebug() << "MEOW: " + this->core->fcnEndBbs(offset);

    // Update disasm side bar
    this->fill_refs(refs, xrefs, data);
}

void MemoryWidget::fill_refs(QList<XrefDescription> refs, QList<XrefDescription> xrefs, QList<int> graph_data)
{
    this->xreFromTreeWidget_2->clear();
    for (int i = 0; i < refs.size(); ++i)
    {
        XrefDescription xref = refs[i];
        QTreeWidgetItem *tempItem = new QTreeWidgetItem();
        tempItem->setText(0, RAddressString(xref.to));
        tempItem->setText(1, core->disassembleSingleInstruction(xref.from));
        tempItem->setData(0, Qt::UserRole, QVariant::fromValue(xref));
        QString tooltip = this->core->cmd("pdi 10 @ " + QString::number(xref.to)).trimmed();
        tempItem->setToolTip(0, tooltip);
        tempItem->setToolTip(1, tooltip);
        this->xreFromTreeWidget_2->insertTopLevelItem(0, tempItem);
    }
    // Adjust columns to content
    int count = this->xreFromTreeWidget_2->columnCount();
    for (int i = 0; i != count; ++i)
    {
        this->xreFromTreeWidget_2->resizeColumnToContents(i);
    }

    this->xrefToTreeWidget_2->clear();
    for (int i = 0; i < xrefs.size(); ++i)
    {
        XrefDescription xref = xrefs[i];

        QTreeWidgetItem *tempItem = new QTreeWidgetItem();
        tempItem->setText(0, RAddressString(xref.from));
        tempItem->setText(1, core->disassembleSingleInstruction(xref.from));
        tempItem->setData(0, Qt::UserRole, QVariant::fromValue(xref));
        QString tooltip = this->core->cmd("pdi 10 @ " + QString::number(xref.from)).trimmed();
        tempItem->setToolTip(0, this->core->cmd("pdi 10 @ " + tooltip).trimmed());
        tempItem->setToolTip(1, this->core->cmd("pdi 10 @ " + tooltip).trimmed());
        this->xrefToTreeWidget_2->insertTopLevelItem(0, tempItem);
    }
    // Adjust columns to content
    int count2 = this->xrefToTreeWidget_2->columnCount();
    for (int i = 0; i != count2; ++i)
    {
        this->xrefToTreeWidget_2->resizeColumnToContents(i);
    }

    // Add data to HTML Polar functions graph
    QFile html(":/html/fcn_graph.html");
    if (!html.open(QIODevice::ReadOnly))
    {
        QMessageBox::information(this, "error", html.errorString());
    }
    QString code = html.readAll();
    html.close();

    QString data = QString("\"%1\", \"%2\", \"%3\", \"%4\", \"%5\"").arg(graph_data.at(2)).arg(graph_data.at(0)).arg(graph_data.at(3)).arg(graph_data.at(1)).arg(graph_data.at(4));
    code.replace("MEOW", data);
    ui->fcnWebView->setHtml(code);

    // Add data to HTML Radar functions graph
    QFile html2(":/html/fcn_radar.html");
    if (!html2.open(QIODevice::ReadOnly))
    {
        QMessageBox::information(this, "error", html.errorString());
    }
    QString code2 = html2.readAll();
    html2.close();

    QString data2 = QString("%1, %2, %3, %4, %5").arg(graph_data.at(2)).arg(graph_data.at(0)).arg(graph_data.at(3)).arg(graph_data.at(1)).arg(graph_data.at(4));
    code2.replace("MEOW", data2);
    ui->radarGraphWebView->setHtml(code2);
}

void MemoryWidget::fillOffsetInfo(QString off)
{
    ui->offsetTreeWidget->clear();
    QString raw = this->core->getOffsetInfo(off);
    QList<QString> lines = raw.split("\n", QString::SkipEmptyParts);
    foreach (QString line, lines)
    {
        QList<QString> eles = line.split(":", QString::SkipEmptyParts);
        QTreeWidgetItem *tempItem = new QTreeWidgetItem();
        tempItem->setText(0, eles.at(0).toUpper());
        tempItem->setText(1, eles.at(1));
        ui->offsetTreeWidget->insertTopLevelItem(0, tempItem);
    }

    // Adjust column to contents
    int count = ui->offsetTreeWidget->columnCount();
    for (int i = 0; i != count; ++i)
    {
        ui->offsetTreeWidget->resizeColumnToContents(i);
    }

    // Add opcode description
    QStringList description = this->core->cmd("?d. @ " + off).split(": ");
    if (description.length() >= 2)
    {
        ui->opcodeDescText->setPlainText("# " + description[0] + ":\n" + description[1]);
    }
}

QString MemoryWidget::normalize_addr(QString addr)
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

void MemoryWidget::setFcnName(RVA addr)
{
    RAnalFunction *fcn;
    QString addr_string;

    fcn = this->core->functionAt(addr);
    if (fcn)
    {
        QString segment = this->core->cmd("S. @ " + QString::number(addr)).split(" ").last();
        addr_string = segment.trimmed() + ":" + fcn->name;
    }
    else
    {
        addr_string = core->cmdFunctionAt(addr);
    }

    ui->fcnNameEdit->setText(addr_string);
}

QString MemoryWidget::normalizeAddr(QString addr)
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

void MemoryWidget::setMiniGraph(QString at)
{
    QString dot = this->core->getSimpleGraph(at);
    //QString dot = this->core->cmd("agc " + at);
    // Add data to HTML Polar functions graph
    QFile html(":/html/graph.html");
    if (!html.open(QIODevice::ReadOnly))
    {
        QMessageBox::information(this, "error", html.errorString());
    }
    QString code = html.readAll();
    html.close();

    code.replace("MEOW", dot);
    ui->webSimpleGraph->setHtml(code);

}

void MemoryWidget::on_polarToolButton_clicked()
{
    ui->radarToolButton->setChecked(false);
    ui->fcnGraphTabWidget->setCurrentIndex(0);
}

void MemoryWidget::on_radarToolButton_clicked()
{
    ui->polarToolButton->setChecked(false);
    ui->fcnGraphTabWidget->setCurrentIndex(1);
}


void MemoryWidget::on_hexSideTab_2_currentChanged(int /*index*/)
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

void MemoryWidget::on_memSideToolButton_clicked()
{
    if (ui->memSideToolButton->isChecked())
    {
        ui->memSideTabWidget_2->hide();
        ui->hexSideTab_2->hide();
        ui->memSideToolButton->setIcon(QIcon(":/img/icons/left_light.svg"));
    }
    else
    {
        ui->memSideTabWidget_2->show();
        ui->hexSideTab_2->show();
        ui->memSideToolButton->setIcon(QIcon(":/img/icons/right_light.svg"));
    }
}

void MemoryWidget::on_previewToolButton_clicked()
{
    ui->memPreviewTab->setCurrentIndex(0);
}

void MemoryWidget::on_decoToolButton_clicked()
{
    ui->memPreviewTab->setCurrentIndex(1);
}

void MemoryWidget::on_simpleGrapgToolButton_clicked()
{
    ui->memPreviewTab->setCurrentIndex(2);
}

void MemoryWidget::on_previewToolButton_2_clicked()
{
    if (ui->previewToolButton_2->isChecked())
    {
        ui->frame_3->setVisible(true);
    }
    else
    {
        ui->frame_3->setVisible(false);
    }
}

void MemoryWidget::resizeEvent(QResizeEvent *event)
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

void MemoryWidget::setScrollMode()
{
    qhelpers::setVerticalScrollMode(ui->xreFromTreeWidget_2);
    qhelpers::setVerticalScrollMode(ui->xrefToTreeWidget_2);
}

void MemoryWidget::on_copyMD5_clicked()
{
    QString md5 = ui->bytesMD5->text();
    QClipboard *clipboard = QApplication::clipboard();
    clipboard->setText(md5);
    // FIXME
    // this->main->addOutput("MD5 copied to clipboard: " + md5);
}

void MemoryWidget::on_copySHA1_clicked()
{
    QString sha1 = ui->bytesSHA1->text();
    QClipboard *clipboard = QApplication::clipboard();
    clipboard->setText(sha1);
    // FIXME
    // this->main->addOutput("SHA1 copied to clipboard: " + sha1);
}

void MemoryWidget::switchTheme(bool dark)
{
    if (dark)
    {
        ui->webSimpleGraph->page()->setBackgroundColor(QColor(64, 64, 64));
    }
    else
    {
        ui->webSimpleGraph->page()->setBackgroundColor(QColor(255, 255, 255));
    }
}

void MemoryWidget::selectHexPreview()
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

void MemoryWidget::seek_back()
{
    //this->main->add_debug_output("Back!");
    // FIXME
    // this->main->backButton_clicked();
}

void MemoryWidget::on_memTabWidget_currentChanged(int /*index*/)
{
    /*this->main->add_debug_output("Update index: " + QString::number(index) + " to function: " + RAddressString(main->getCursorAddress()));
    this->main->add_debug_output("Last disasm: " + RAddressString(this->last_disasm_fcn));
    this->main->add_debug_output("Last graph: " + RAddressString(this->last_graph_fcn));
    this->main->add_debug_output("Last hexdump: " + RAddressString(this->last_hexdump_fcn));*/
    this->updateViews(RVA_INVALID);
}

void MemoryWidget::updateViews(RVA offset)
{
    // Update only the selected view to improve performance

    int index = ui->memTabWidget->tabBar()->currentIndex();

    // Anyway updateViews will die after break this widget.
    // FIXME? One cursor per widget ? (if not synced)
    // RVA cursor_addr = main->getCursorAddress();
    // QString cursor_addr_string = RAddressString(cursor_addr);
    QString cursor_addr_string = core->cmd("s");
    RVA cursor_addr = cursor_addr_string.toULongLong();

    if (offset != RVA_INVALID)
        next_disasm_top_offset = offset;

    if (index == 1)
    {
        // Hex
        if (this->last_hexdump_fcn != cursor_addr)
        {
            this->refreshHexdump(cursor_addr_string);
            this->last_hexdump_fcn = cursor_addr;
        }
    }
    // TODO WTF
}

void MemoryWidget::showOffsets(bool show)
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
