#include "Notepad.h"
#include "ui_Notepad.h"

#include "MainWindow.h"

#include "utils/MdHighlighter.h"
#include "utils/Highlighter.h"

#include <QPlainTextEdit>
#include <QFont>
#include <QDebug>
#include <QFontDialog>
#include <QTextCursor>
#include <QMenu>


Notepad::Notepad(MainWindow *main, QWidget *parent) :
    DockWidget(parent),
    ui(new Ui::Notepad)
{
    ui->setupUi(this);

    // Radare core found in:
    this->main = main;

    highlighter = new MdHighlighter(ui->notepadTextEdit->document());
    ui->splitter->setStretchFactor(0, 2);
    isFirstTime = true;
    this->notesTextEdit = ui->notepadTextEdit;

    // Increase notes document inner margin
    QTextDocument *docu = this->notesTextEdit->document();
    docu->setDocumentMargin(10);
    // Increase preview notes document inner margin
    QPlainTextEdit *preview = ui->previewTextEdit;
    QTextDocument *preview_docu = preview->document();
    preview_docu->setDocumentMargin(10);

    // Context menu
    ui->notepadTextEdit->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(ui->notepadTextEdit, SIGNAL(customContextMenuRequested(const QPoint &)),
            this, SLOT(showNotepadContextMenu(const QPoint &)));
}

Notepad::~Notepad() {}

void Notepad::setup()
{
}

void Notepad::refresh()
{
    // TODO: implement
    eprintf("%s - not implemented\n", Q_FUNC_INFO);
}

void Notepad::setText(const QString &str)
{
    ui->notepadTextEdit->setPlainText(str);
}

QString Notepad::textToBase64() const
{
    return notesTextEdit->toPlainText().toUtf8().toBase64();
}

void Notepad::appendPlainText(const QString &text)
{
    notesTextEdit->appendPlainText(text);
}

void Notepad::on_fontButton_clicked()
{
    bool ok = true;

    QFont font = QFontDialog::getFont(&ok, ui->notepadTextEdit->font(), this) ;
    if (ok)
    {
        // the user clicked OK and font is set to the font the user selected
        //ui->notepadTextEdit->setFont(font);
        //ui->previewTextEdit->setFont(font);
        this->setFonts(font);
    }
}

void Notepad::setFonts(QFont font)
{
    ui->notepadTextEdit->setFont(font);
    ui->previewTextEdit->setFont(font);
}

void Notepad::on_boldButton_clicked()
{
    QTextCursor cursor = ui->notepadTextEdit->textCursor();
    if (cursor.hasSelection())
    {
        QString text = cursor.selectedText();
        cursor.removeSelectedText();
        cursor.insertText("**" + text + "**");
    }
    else
    {
        cursor.insertText("****");
    }
}

void Notepad::on_italicsButton_clicked()
{
    QTextCursor cursor = ui->notepadTextEdit->textCursor();
    if (cursor.hasSelection())
    {
        QString text = cursor.selectedText();
        cursor.removeSelectedText();
        cursor.insertText("*" + text + "*");
    }
    else
    {
        cursor.insertText("**");
    }
}

void Notepad::on_h1Button_clicked()
{
    QTextCursor cursor = ui->notepadTextEdit->textCursor();
    if (cursor.hasSelection())
    {
        QString text = cursor.selectedText();
        cursor.removeSelectedText();
        cursor.insertText("# " + text);
    }
    else
    {
        cursor.insertText("# ");
    }
}

void Notepad::on_h2Button_clicked()
{
    QTextCursor cursor = ui->notepadTextEdit->textCursor();
    if (cursor.hasSelection())
    {
        QString text = cursor.selectedText();
        cursor.removeSelectedText();
        cursor.insertText("## " + text);
    }
    else
    {
        cursor.insertText("## ");
    }
}

void Notepad::on_h3Button_clicked()
{
    QTextCursor cursor = ui->notepadTextEdit->textCursor();
    if (cursor.hasSelection())
    {
        QString text = cursor.selectedText();
        cursor.removeSelectedText();
        cursor.insertText("### " + text);
    }
    else
    {
        cursor.insertText("### ");
    }
}

void Notepad::on_undoButton_clicked()
{
    QTextCursor cursor = ui->notepadTextEdit->textCursor();
    QTextDocument *doc = ui->notepadTextEdit->document();
    doc->undo();
}

void Notepad::on_redoButton_clicked()
{
    QTextCursor cursor = ui->notepadTextEdit->textCursor();
    QTextDocument *doc = ui->notepadTextEdit->document();
    doc->redo();
}

void Notepad::highlightPreview()
{
    disasm_highlighter = new Highlighter(ui->previewTextEdit->document());
}

void Notepad::on_searchEdit_returnPressed()
{
    QString searchString = ui->searchEdit->text();
    QTextDocument *document = ui->notepadTextEdit->document();

    if (isFirstTime == false)
        document->undo();

    if (!searchString.isEmpty())
    {

        QTextCursor highlightCursor(document);
        QTextCursor cursor(document);

        cursor.beginEditBlock();

        QTextCharFormat plainFormat(highlightCursor.charFormat());
        QTextCharFormat colorFormat = plainFormat;
        colorFormat.setForeground(Qt::red);

        while (!highlightCursor.isNull() && !highlightCursor.atEnd())
        {
            highlightCursor = document->find(searchString, highlightCursor, QTextDocument::FindWholeWords);

            if (!highlightCursor.isNull())
            {
                highlightCursor.movePosition(QTextCursor::WordRight,
                                             QTextCursor::KeepAnchor);
                highlightCursor.mergeCharFormat(colorFormat);
            }
        }

        cursor.endEditBlock();
        isFirstTime = false;
    }
}

void Notepad::on_searchEdit_textEdited(const QString &arg1)
{
    Q_UNUSED(arg1);

    QString searchString = ui->searchEdit->text();
    QTextDocument *document = ui->notepadTextEdit->document();

    if (isFirstTime == false)
        document->undo();

    if (!searchString.isEmpty())
    {

        QTextCursor highlightCursor(document);
        QTextCursor cursor(document);

        cursor.beginEditBlock();

        QTextCharFormat plainFormat(highlightCursor.charFormat());
        QTextCharFormat colorFormat = plainFormat;
        colorFormat.setForeground(Qt::red);

        while (!highlightCursor.isNull() && !highlightCursor.atEnd())
        {
            highlightCursor = document->find(searchString, highlightCursor);

            if (!highlightCursor.isNull())
            {
                //highlightCursor.movePosition(QTextCursor::WordRight,
                //                       QTextCursor::KeepAnchor);
                highlightCursor.mergeCharFormat(colorFormat);
            }
        }

        cursor.endEditBlock();
        isFirstTime = false;
    }
}

void Notepad::on_searchEdit_textChanged(const QString &arg1)
{
    Q_UNUSED(arg1);

    QString searchString = ui->searchEdit->text();
    QTextDocument *document = ui->notepadTextEdit->document();

    if (isFirstTime == false)
        document->undo();

    if (!searchString.isEmpty())
    {

        QTextCursor highlightCursor(document);
        QTextCursor cursor(document);

        cursor.beginEditBlock();

        QTextCharFormat plainFormat(highlightCursor.charFormat());
        QTextCharFormat colorFormat = plainFormat;
        colorFormat.setForeground(Qt::red);

        while (!highlightCursor.isNull() && !highlightCursor.atEnd())
        {
            highlightCursor = document->find(searchString, highlightCursor);

            if (!highlightCursor.isNull())
            {
                //highlightCursor.movePosition(QTextCursor::WordRight,
                //                       QTextCursor::KeepAnchor);
                highlightCursor.mergeCharFormat(colorFormat);
            }
        }

        cursor.endEditBlock();
        isFirstTime = false;
    }
}

void Notepad::showNotepadContextMenu(const QPoint &pt)
{
    // Set Notepad popup menu
    QMenu *menu = ui->notepadTextEdit->createStandardContextMenu();
    QTextCursor cur = ui->notepadTextEdit->textCursor();
    QAction *first = menu->actions().at(0);

    if (cur.hasSelection())
    {
        // Get selected text
        //this->main->add_debug_output("Selected text: " + cur.selectedText());
        this->addr = cur.selectedText();
    }
    else
    {
        // Get word under the cursor
        cur.select(QTextCursor::WordUnderCursor);
        //this->main->add_debug_output("Word: " + cur.selectedText());
        this->addr = cur.selectedText();
    }
    ui->actionDisassmble_bytes->setText("Disassemble bytes at: " + this->addr);
    ui->actionDisassmble_function->setText("Disassemble function at: " + this->addr);
    ui->actionHexdump_bytes->setText("Hexdump bytes at: " + this->addr);
    ui->actionCompact_Hexdump->setText("Compact Hexdump at: " + this->addr);
    ui->actionHexdump_function->setText("Hexdump function at: " + this->addr);
    menu->insertAction(first, ui->actionDisassmble_bytes);
    menu->insertAction(first, ui->actionDisassmble_function);
    menu->insertAction(first, ui->actionHexdump_bytes);
    menu->insertAction(first, ui->actionCompact_Hexdump);
    menu->insertAction(first, ui->actionHexdump_function);
    menu->insertSeparator(first);
    ui->notepadTextEdit->setContextMenuPolicy(Qt::DefaultContextMenu);
    menu->exec(ui->notepadTextEdit->mapToGlobal(pt));
    delete menu;
    ui->notepadTextEdit->setContextMenuPolicy(Qt::CustomContextMenu);
}

void Notepad::on_actionDisassmble_bytes_triggered()
{
    ui->previewTextEdit->setPlainText(CutterCore::getInstance()->cmd("pd 100 @ " + this->addr));
}

void Notepad::on_actionDisassmble_function_triggered()
{
    ui->previewTextEdit->setPlainText(CutterCore::getInstance()->cmd("pdf @ " + this->addr));
}

void Notepad::on_actionHexdump_bytes_triggered()
{
    ui->previewTextEdit->setPlainText(CutterCore::getInstance()->cmd("px 1024 @ " + this->addr));
}

void Notepad::on_actionCompact_Hexdump_triggered()
{
    ui->previewTextEdit->setPlainText(CutterCore::getInstance()->cmd("pxi 1024 @ " + this->addr));
}

void Notepad::on_actionHexdump_function_triggered()
{
    ui->previewTextEdit->setPlainText(CutterCore::getInstance()->cmd("pxf @ " + this->addr));
}
