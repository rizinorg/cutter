
#ifndef HEXDUMPWIDGET_H
#define HEXDUMPWIDGET_H


#include <QDebug>
#include <QTextEdit>
#include <QDockWidget>
#include <QTreeWidget>
#include <QTabWidget>
#include <QUrl>
#include <QPlainTextEdit>
#include <QMouseEvent>
#include <memory>
#include "cutter.h"
#include "utils/Highlighter.h"
#include "utils/HexAsciiHighlighter.h"
#include "utils/HexHighlighter.h"
#include "Dashboard.h"


namespace Ui
{
    class HexdumpWidget;
}

class HexdumpWidget : public QDockWidget
{
Q_OBJECT

public:
    explicit HexdumpWidget(const QString &title, QWidget *parent = nullptr, Qt::WindowFlags flags = 0);
    explicit HexdumpWidget(QWidget *parent = nullptr, Qt::WindowFlags flags = 0);
    ~HexdumpWidget();

    QTextEdit        *hexOffsetText;
    QTextEdit        *hexASCIIText;
    QTextEdit        *hexHexText;

    QPlainTextEdit   *hexDisasTextEdit;

    Highlighter        *highlighter;

signals:
    void fontChanged(QFont font);

public slots:
    void fillPlugins();

    QString normalize_addr(QString addr);

    QString normalizeAddr(QString addr);

    void selectHexPreview();

    void showOffsets(bool show);

protected:
    void resizeEvent(QResizeEvent *event) override;

private:
    std::unique_ptr<Ui::HexdumpWidget> ui;
    CutterCore *core;

    ut64 hexdumpBottomOffset;

    void refresh(RVA addr = RVA_INVALID);

private slots:
    void on_seekChanged(RVA addr);

    void highlightHexCurrentLine();
    void setFonts(QFont font);

    void highlightHexWords(const QString &str);
    void on_actionSettings_menu_1_triggered();
    void on_actionHideHexdump_side_panel_triggered();

    void showHexdumpContextMenu(const QPoint &pt);
    void showHexASCIIContextMenu(const QPoint &pt);

    void on_hexHexText_2_selectionChanged();
    void on_hexArchComboBox_2_currentTextChanged(const QString &arg1);
    void on_hexBitsComboBox_2_currentTextChanged(const QString &arg1);

    void on_action1column_triggered();
    void on_action2columns_triggered();
    void on_action4columns_triggered();
    void on_action8columns_triggered();
    void on_action16columns_triggered();
    void on_action32columns_triggered();
    void on_action64columns_triggered();

    void resizeHexdump();
    void hexScrolled();
    QList<QString> get_hexdump(const QString &offset);

    void on_codeCombo_2_currentTextChanged(const QString &arg1);
    void on_hexSideTab_2_currentChanged(int index);
    void on_memSideToolButton_clicked();
    void on_copyMD5_clicked();
    void on_copySHA1_clicked();
};

#endif // HEXDUMPWIDGET_H
