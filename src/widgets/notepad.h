#ifndef NOTEPAD_H
#define NOTEPAD_H

#include "dockwidget.h"

class MainWindow;
class MdHighlighter;
class Highlighter;
class QPlainTextEdit;

namespace Ui
{
    class Notepad;
}

class Notepad : public DockWidget
{
    Q_OBJECT

public:
    explicit Notepad(MainWindow *main, QWidget *parent = 0);
    ~Notepad();

    void setup() override;

    void refresh() override;

    void setText(const QString &str);
    QString textToBase64() const;

    void appendPlainText(const QString &text);

    void highlightPreview();

public slots:
    void setFonts(QFont font);

private slots:
    void on_fontButton_clicked();

    void on_boldButton_clicked();

    void on_italicsButton_clicked();

    void on_h1Button_clicked();

    void on_h2Button_clicked();

    void on_h3Button_clicked();

    void on_undoButton_clicked();

    void on_redoButton_clicked();

    void on_searchEdit_returnPressed();

    void on_searchEdit_textEdited(const QString &arg1);

    void on_searchEdit_textChanged(const QString &arg1);

    void showNotepadContextMenu(const QPoint &pt);

    void on_actionDisassmble_bytes_triggered();

    void on_actionDisassmble_function_triggered();

    void on_actionHexdump_bytes_triggered();

    void on_actionCompact_Hexdump_triggered();

    void on_actionHexdump_function_triggered();

private:
    Ui::Notepad         *ui;
    MdHighlighter       *highlighter;
    Highlighter         *disasm_highlighter;
    bool                isFirstTime;
    MainWindow          *main;
    QString             addr;
    QPlainTextEdit      *notesTextEdit;
};

#endif // NOTEPAD_H
