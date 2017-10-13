#include "DisassemblyWidget.h"
#include "menus/DisassemblyContextMenu.h"
#include "dialogs/XrefsDialog.h"
#include "utils/HexAsciiHighlighter.h"
#include "utils/HexHighlighter.h"
#include <QShortcut>
#include <QScrollBar>

DisassemblyWidget::DisassemblyWidget(QWidget *parent) :
    QDockWidget(parent),
    mDisasTextEdit(new QTextEdit(this))
{
    // Configure Dock
    setWidget(mDisasTextEdit);
    setAllowedAreas(Qt::AllDockWidgetAreas);
    setObjectName("DisassemblyWidget");

    // TODO Use Settings
    mDisasTextEdit->setFont(QFont("Monospace", 10));

    // Increase asm text edit margin
    QTextDocument *asm_docu = mDisasTextEdit->document();
    asm_docu->setDocumentMargin(10);

    // Setup disasm highlight
    connect(mDisasTextEdit, SIGNAL(cursorPositionChanged()), this, SLOT(highlightCurrentLine()));
    highlightCurrentLine();

    // Event filter to intercept double clicks in the textbox
    mDisasTextEdit->viewport()->installEventFilter(this);

    // Set Disas context menu
    mDisasTextEdit->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(mDisasTextEdit, SIGNAL(customContextMenuRequested(const QPoint &)),
            this, SLOT(showDisasContextMenu(const QPoint &)));

    // x or X to show XRefs
    connect(new QShortcut(QKeySequence(Qt::Key_X), mDisasTextEdit),
            SIGNAL(activated()), this, SLOT(showXrefsDialog()));
    connect(new QShortcut(Qt::SHIFT + Qt::Key_X, mDisasTextEdit),
            SIGNAL(activated()), this, SLOT(showXrefsDialog()));

    // Scrollbar
    connect(mDisasTextEdit->verticalScrollBar(), SIGNAL(valueChanged(int)), this, SLOT(disasmScrolled()));

    // TODO Shortcuts
    // Semicolon to add comment
    //QShortcut *comment_shortcut = new QShortcut(QKeySequence(Qt::Key_Semicolon), mDisasTextEdit);
    //connect(comment_shortcut, SIGNAL(activated()), this, SLOT(on_actionDisasAdd_comment_triggered()));
    //comment_shortcut->setContext(Qt::WidgetShortcut);

    // N to rename function
    //QShortcut *rename_shortcut = new QShortcut(QKeySequence(Qt::Key_N), mDisasTextEdit);
    //connect(rename_shortcut, SIGNAL(activated()), this, SLOT(on_actionFunctionsRename_triggered()));
    //rename_shortcut->setContext(Qt::WidgetShortcut);

    // Esc to seek back
    //QShortcut *back_shortcut = new QShortcut(QKeySequence(Qt::Key_Escape), mDisasTextEdit);
    //connect(back_shortcut, SIGNAL(activated()), this, SLOT(seek_back()));
    //back_shortcut->setContext(Qt::WidgetShortcut);

    // CTRL + R to refresh the disasm
    //QShortcut *refresh_shortcut = new QShortcut(QKeySequence(Qt::CTRL + Qt::Key_R), mDisasTextEdit);
    //connect(refresh_shortcut, SIGNAL(activated()), this, SLOT(refreshDisasm()));
    //refresh_shortcut->setContext(Qt::WidgetShortcut);

    // Seek signal
    connect(CutterCore::getInstance(), SIGNAL(seekChanged(RVA)), this, SLOT(on_seekChanged(RVA)));
}

DisassemblyWidget::DisassemblyWidget(const QString &title, QWidget *parent) :
    DisassemblyWidget(parent)
{
    this->setWindowTitle(title);
}
void DisassemblyWidget::highlightCurrentLine()
{
    QList<QTextEdit::ExtraSelection> extraSelections;

    // Highlight the current line in yellow
    if (mDisasTextEdit->isReadOnly())
    {
        QTextEdit::ExtraSelection selection;

        QColor lineColor = QColor(190, 144, 212);

        selection.format.setBackground(lineColor);
        selection.format.setProperty(QTextFormat::FullWidthSelection, true);
        selection.cursor = mDisasTextEdit->textCursor();
        selection.cursor.clearSelection();
        extraSelections.append(selection);
    }

    // Highlight the current word
    QTextCursor cursor = mDisasTextEdit->textCursor();
    cursor.select(QTextCursor::WordUnderCursor);

    QTextEdit::ExtraSelection currentWord;

    QColor blueColor = QColor(Qt::blue).lighter(160);
    currentWord.format.setBackground(blueColor);

    currentWord.cursor = cursor;
    extraSelections.append(currentWord);
    currentWord.cursor.clearSelection();

    // Highlight all the words in the document same as the actual one
    QString searchString = cursor.selectedText();
    QTextDocument *document = mDisasTextEdit->document();

    //QTextCursor highlightCursor(document);
    QTextEdit::ExtraSelection highlightSelection;
    highlightSelection.cursor = cursor;
    highlightSelection.format.setBackground(blueColor);
    QTextCursor cursor2(document);

    cursor2.beginEditBlock();

    highlightSelection.cursor.movePosition(QTextCursor::Start, QTextCursor::MoveAnchor);
    while (!highlightSelection.cursor.isNull() && !highlightSelection.cursor.atEnd())
    {
        highlightSelection.cursor = document->find(searchString, highlightSelection.cursor, QTextDocument::FindWholeWords);

        if (!highlightSelection.cursor.isNull())
        {
            highlightSelection.cursor.movePosition(QTextCursor::EndOfWord, QTextCursor::KeepAnchor);
            extraSelections.append(highlightSelection);
        }
    }
    cursor2.endEditBlock();

    mDisasTextEdit->setExtraSelections(extraSelections);
}

void DisassemblyWidget::showDisasContextMenu(const QPoint &pt)
{
    DisassemblyContextMenu menu(this->readCurrentDisassemblyOffset(), mDisasTextEdit);
    menu.exec(mDisasTextEdit->mapToGlobal(pt));
}

void DisassemblyWidget::showXrefsDialog()
{
    // Get current offset
    QTextCursor tc = mDisasTextEdit->textCursor();
    tc.select(QTextCursor::LineUnderCursor);
    QString lastline = tc.selectedText();
    QString ele = lastline.split(" ", QString::SkipEmptyParts)[0];
    if (ele.contains("0x"))
    {
        RVA addr = ele.toLongLong(0, 16);
        XrefsDialog *dialog = new XrefsDialog(this);
        dialog->fillRefsForAddress(addr, RAddressString(addr), false);
        dialog->exec();
    }
}

RVA DisassemblyWidget::readCurrentDisassemblyOffset()
{
    // TODO: do this in a different way without parsing the disassembly text
    QTextCursor tc = mDisasTextEdit->textCursor();
    tc.select(QTextCursor::LineUnderCursor);
    QString lastline = tc.selectedText();
    QStringList parts = lastline.split(" ", QString::SkipEmptyParts);

    if (parts.isEmpty())
        return RVA_INVALID;

    QString ele = parts[0];
    if (!ele.contains("0x"))
        return RVA_INVALID;

    return ele.toULongLong(0, 16);
}

bool DisassemblyWidget::loadMoreDisassembly()
{
    /*
     * Add more disasm as the user scrolls
     * Not working properly when scrolling upwards
     * r2 doesn't handle properly 'pd-' for archs with variable instruction size
     */
    // Disconnect scroll signals to add more content
    disconnect(mDisasTextEdit->verticalScrollBar(), SIGNAL(valueChanged(int)), this, SLOT(disasmScrolled()));

    QScrollBar *sb = mDisasTextEdit->verticalScrollBar();

    bool loaded = false;

    if (sb->value() > sb->maximum() - 10)
    {
        //this->main->add_debug_output("End is coming");

        QTextCursor tc = mDisasTextEdit->textCursor();
        tc.movePosition(QTextCursor::End);
        RVA offset = readCurrentDisassemblyOffset();

        if (offset != RVA_INVALID)
        {
            CutterCore::getInstance()->seek(offset);
            QString raw = CutterCore::getInstance()->cmd("pd 200");
            QString txt = raw.section("\n", 1, -1);
            //this->disasTextEdit->appendPlainText(" ;\n ; New content here\n ;\n " + txt.trimmed());
            mDisasTextEdit->append(txt.trimmed());
        }
        else
        {
            tc.movePosition(QTextCursor::End);
            tc.select(QTextCursor::LineUnderCursor);
            QString lastline = tc.selectedText();
            //this->main->addDebugOutput("Last line: " + lastline);
        }

        loaded = true;

        // Code below will be used to append more disasm upwards, one day
    } /* else if (sb->value() < sb->minimum() + 10) {
        //this->main->add_debug_output("Begining is coming");

        QTextCursor tc = this->disasTextEdit->textCursor();
        tc.movePosition( QTextCursor::Start );
        tc.select( QTextCursor::LineUnderCursor );
        QString firstline = tc.selectedText();
        //this->main->add_debug_output("First Line: " + firstline);
        QString ele = firstline.split(" ", QString::SkipEmptyParts)[0];
        //this->main->add_debug_output("First Offset: " + ele);
        if (ele.contains("0x")) {
            int b = this->disasTextEdit->verticalScrollBar()->maximum();
            this->core->cmd("ss " + ele);
            this->core->cmd("so -50");
            QString raw = this->core->cmd("pd 50");
            //this->main->add_debug_output(raw);
            //QString txt = raw.section("\n", 1, -1);
            //this->main->add_debug_output(txt);
            tc.movePosition(QTextCursor::Start, QTextCursor::MoveAnchor);
            //tc.insertText(raw.trimmed() + "\n ;\n ; New content prepended here\n ;\n");
            int c = this->disasTextEdit->verticalScrollBar()->maximum();
            int z = c -b;
            int a = this->disasTextEdit->verticalScrollBar()->sliderPosition();
            this->disasTextEdit->verticalScrollBar()->setValue(a + z);
        } else {
            tc.movePosition( QTextCursor::Start );
            tc.select( QTextCursor::LineUnderCursor );
            QString lastline = tc.selectedText();
            this->main->add_debug_output("Last line: " + lastline);
        }
    } */

    // Reconnect scroll signals
    connect(mDisasTextEdit->verticalScrollBar(), SIGNAL(valueChanged(int)), this, SLOT(disasmScrolled()));

    return loaded;
}


void DisassemblyWidget::disasmScrolled()
{
    loadMoreDisassembly();
}

void DisassemblyWidget::refreshDisasm()
{
    // TODO Very slow mostly because of the highlight
    // Prevent further scroll
    disconnect(mDisasTextEdit->verticalScrollBar(), SIGNAL(valueChanged(int)), this, SLOT(disasmScrolled()));
    disconnect(mDisasTextEdit, SIGNAL(cursorPositionChanged()), this, SLOT(on_mDisasTextEdit_cursorPositionChanged()));

    QString disas = CutterCore::getInstance()->cmd("pd 200");

    mDisasTextEdit->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);

    mDisasTextEdit->setPlainText(disas.trimmed());

    auto cursor = mDisasTextEdit->textCursor();
    cursor.setPosition(0);
    mDisasTextEdit->setTextCursor(cursor);
    mDisasTextEdit->verticalScrollBar()->setValue(0);

    // load more disassembly if necessary
    /*static const int load_more_limit = 10; // limit passes, so it can't take forever
    for (int load_more_i = 0; load_more_i < load_more_limit; load_more_i++)
    {
        if (!loadMoreDisassembly())
            break;
        mDisasTextEdit->verticalScrollBar()->setValue(0);
    }*/

    connect(mDisasTextEdit->verticalScrollBar(), SIGNAL(valueChanged(int)), this, SLOT(disasmScrolled()));
    connect(mDisasTextEdit, SIGNAL(cursorPositionChanged()), this, SLOT(on_mDisasTextEdit_cursorPositionChanged()));
    //this->on_mDisasTextEdit_cursorPositionChanged();

    this->highlightDisasms();
}

void DisassemblyWidget::on_mDisasTextEdit_cursorPositionChanged()
{
    // Get current offset
    QTextCursor tc = mDisasTextEdit->textCursor();
    tc.select(QTextCursor::LineUnderCursor);
    QString lastline = tc.selectedText().trimmed();
    QList<QString> words = lastline.split(" ", QString::SkipEmptyParts);
    if (words.length() == 0)
    {
        return;
    }
    QString ele = words[0];
    // TODO
    /*if (ele.contains("0x"))
    {
        this->fillOffsetInfo(ele);
        QString at = this->core->cmdFunctionAt(ele);
        QString deco = this->core->getDecompiledCode(at);


        RVA addr = ele.midRef(2).toULongLong(0, 16);
        // FIXME per widget CursorAddress no?
        // this->main->setCursorAddress(addr);

        if (deco != "")
        {
            ui->decoTextEdit->setPlainText(deco);
        }
        else
        {
            ui->decoTextEdit->setPlainText("");
        }
        // Get jump information to fill the preview
        QString jump =  this->core->getOffsetJump(ele);
        if (!jump.isEmpty())
        {
            // Fill the preview
            QString jump_code = this->core->cmd("pdf @ " + jump);
            ui->previewTextEdit->setPlainText(jump_code.trimmed());
            ui->previewTextEdit->moveCursor(QTextCursor::End);
            ui->previewTextEdit->find(jump.trimmed(), QTextDocument::FindBackward);
            ui->previewTextEdit->moveCursor(QTextCursor::StartOfWord, QTextCursor::MoveAnchor);
        }
        else
        {
            ui->previewTextEdit->setPlainText("");
        }
        //this->main->add_debug_output("Fcn at: '" + at + "'");
        if (this->last_fcn != at)
        {
            this->last_fcn = at;
            //this->main->add_debug_output("New Fcn: '" + this->last_fcn + "'");
            // Refresh function information at sidebar
            ui->fcnNameEdit->setText(at);
            // FIXME TITLE?
            // this->main->previewDock->setWindowTitle(at);
            //this->main->previewDock->create_graph(ele);
            this->setMiniGraph(at);
        }
    }
    */
}

bool DisassemblyWidget::eventFilter(QObject *obj, QEvent *event)
{
    if ((obj == mDisasTextEdit || obj == mDisasTextEdit->viewport()) && event->type() == QEvent::MouseButtonDblClick)
    {
        QMouseEvent *mouseEvent = static_cast<QMouseEvent *>(event);
        //qDebug()<<QString("Click location: (%1,%2)").arg(mouseEvent->x()).arg(mouseEvent->y());
        QTextCursor cursor = mDisasTextEdit->cursorForPosition(QPoint(mouseEvent->x(), mouseEvent->y()));
        cursor.select(QTextCursor::LineUnderCursor);
        QString lastline = cursor.selectedText();
        auto eles = lastline.split(" ", QString::SkipEmptyParts);
        QString ele = eles.isEmpty() ? "" : eles[0];
        if (ele.contains("0x"))
        {
            QString jump = CutterCore::getInstance()->getOffsetJump(ele);
            if (!jump.isEmpty())
            {
                if (jump.contains("0x"))
                {
                    QString fcn = CutterCore::getInstance()->cmdFunctionAt(jump);
                    if (!fcn.isEmpty())
                    {
                        RVA addr = jump.trimmed().toULongLong(0, 16);
                        CutterCore::getInstance()->seek(addr);
                    }
                }
                else
                {
                    RVA addr = CutterCore::getInstance()->cmd("?v " + jump).toULongLong(0, 16);
                    CutterCore::getInstance()->seek(addr);
                }
            }
        }
    }
    return QDockWidget::eventFilter(obj, event);
}

void DisassemblyWidget::on_seekChanged(RVA offset)
{
    Q_UNUSED(offset);
    refreshDisasm();
}

void DisassemblyWidget::highlightDisasms()
{
    // TODO Improve this syntax Highlighting
    // TODO Must be usable for the graph view
    //Highlighter *highlighter = new Highlighter(mDisasTextEdit->document());
    //Highlighter *highlighter_5 = new Highlighter(mDisasTextEdit->document());
    //AsciiHighlighter *ascii_highlighter = new AsciiHighlighter(mDisasTextEdit->document());
    //HexHighlighter *hex_highlighter = new HexHighlighter(mDisasTextEdit->document());
    //Highlighter *preview_highlighter = new Highlighter(mDisasTextEdit->document());
    //Highlighter *deco_highlighter = new Highlighter(mDisasTextEdit->document());
}
