#ifndef NOTEPAD_H
#define NOTEPAD_H

#include <memory>
#include <QDockWidget>

class MainWindow;
class MdHighlighter;
class Highlighter;
class QPlainTextEdit;

namespace Ui
{
    class Notepad;
}

class Notepad : public QDockWidget
{
    Q_OBJECT

public:
    explicit Notepad(MainWindow *main, QWidget *parent = 0);
    ~Notepad();

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

    void updateNotes(const QString &notes);
    void textChanged();

private:
    std::unique_ptr<Ui::Notepad> ui;
    MdHighlighter       *highlighter;
    Highlighter         *disasm_highlighter;
    bool                isFirstTime;
    MainWindow          *main;
    QString             addr;
    QPlainTextEdit      *notesTextEdit;
};

#endif // NOTEPAD_H
