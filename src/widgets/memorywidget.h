#ifndef MEMORYWIDGET_H
#define MEMORYWIDGET_H

#include "qrcore.h"
#include "highlighter.h"
#include "hexascii_highlighter.h"
#include "hexhighlighter.h"

#include <QDebug>
#include <QTextEdit>
#include <QDockWidget>
#include <QTreeWidget>
#include <QTabWidget>
#include <QWebEngineView>
#include <QUrl>
#include <QPlainTextEdit>
#include <QMouseEvent>

class MainWindow;

namespace Ui
{
    class MemoryWidget;
}

class MemoryWidget : public QDockWidget
{
    Q_OBJECT

public:
    explicit MemoryWidget(MainWindow *main);
    ~MemoryWidget();

    MainWindow       *main;
    QPlainTextEdit   *disasTextEdit;
    QTextEdit        *hexOffsetText;
    QPlainTextEdit   *hexDisasTextEdit;
    QTextEdit        *hexASCIIText;
    QTextEdit        *hexHexText;
    QTreeWidget      *xrefToTreeWidget_2;
    QTreeWidget      *xreFromTreeWidget_2;
    QTabWidget       *memTabWidget;
    QWebEngineView         *graphWebView;
    QWebEngineView         *histoWebView;

    Highlighter        *highlighter;
    Highlighter        *highlighter_5;
    AsciiHighlighter   *ascii_highlighter;
    HexHighlighter     *hex_highlighter;
    Highlighter        *preview_highlighter;
    Highlighter        *deco_highlighter;

public slots:
    void fillPlugins(QStringList plugins);

    void addTextDisasm(QString txt);

    void replaceTextDisasm(QString txt);

    void refreshDisasm(const QString &offset = QString());

    void refreshHexdump(QString where = 0);

    void fill_refs(QList<QStringList> list, QList<QStringList> xrefs, QList<int> graph_data);

    void fillOffsetInfo(QString off);

    void get_refs_data(const QString &offset);

    void seek_to(QString offset);

    void create_graph(QString off);

    QString normalize_addr(QString addr);

    QString normalizeAddr(QString addr);

    void setFcnName(QString addr);

    void setMiniGraph(QString at);

    void switchTheme(bool dark);

    void highlightDisasms();

    void selectHexPreview();

    void frameLoadFinished(bool ok);

    void updateViews();

protected:
    void resizeEvent(QResizeEvent *event) override;
    bool eventFilter(QObject *obj, QEvent *event) override;

private:
    Ui::MemoryWidget *ui;

    ut64 hexdumpTopOffset;
    ut64 hexdumpBottomOffset;
    QString last_fcn;
    QString last_disasm_fcn;
    QString last_graph_fcn;
    QString last_hexdump_fcn;

private slots:
    void highlightCurrentLine();

    void highlightHexCurrentLine();
    void highlightPreviewCurrentLine();
    void highlightDecoCurrentLine();
    void setFonts(QFont font);

    void highlightHexWords(QString str);
    void on_showInfoButton_2_clicked();
    void on_actionSettings_menu_1_triggered();
    void on_actionHideDisasm_side_panel_triggered();
    void on_actionHideHexdump_side_panel_triggered();
    void on_actionHideGraph_side_panel_triggered();
    void on_disButton_2_clicked();
    void on_hexButton_2_clicked();
    void on_graphButton_2_clicked();
    void showDisasContextMenu(const QPoint &pt);
    void showHexdumpContextMenu(const QPoint &pt);
    void showHexASCIIContextMenu(const QPoint &pt);
    void on_actionSend_to_Notepad_triggered();
    void on_actionDisasAdd_comment_triggered();
    void on_actionFunctionsRename_triggered();
    void on_actionDisas_ShowHideBytes_triggered();

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

    void disasmScrolled();
    void resizeHexdump();
    void hexScrolled();
    QList<QString> get_hexdump(QString off);

    //void showDisas();
    void showHexdump();
    //void showGraph();
    void cycleViews();
    void on_xreFromTreeWidget_2_itemDoubleClicked(QTreeWidgetItem *item, int column);
    void on_xrefToTreeWidget_2_itemDoubleClicked(QTreeWidgetItem *item, int column);
    void on_xrefFromToolButton_2_clicked();
    void on_xrefToToolButton_2_clicked();
    void on_codeCombo_2_currentTextChanged(const QString &arg1);
    void on_disasTextEdit_2_cursorPositionChanged();
    void on_offsetToolButton_clicked();
    void on_polarToolButton_clicked();
    void on_radarToolButton_clicked();
    void on_hexSideTab_2_currentChanged(int index);
    void on_memSideToolButton_clicked();
    void on_previewToolButton_clicked();
    void on_decoToolButton_clicked();
    void on_previewToolButton_2_clicked();
    void on_actionXRefs_triggered();
    void on_actionDisasSwitch_case_triggered();
    void on_actionSyntax_AT_T_Intel_triggered();
    void on_actionSeparate_bytes_triggered();
    void on_actionRight_align_bytes_triggered();
    void on_actionSeparate_disasm_calls_triggered();
    void on_actionShow_stack_pointer_triggered();
    void on_copyMD5_clicked();
    void on_copySHA1_clicked();
    void on_simpleGrapgToolButton_clicked();
    void on_opcodeDescButton_clicked();
    void seek_back();
    void on_memTabWidget_currentChanged(int index);
};

#endif // MEMORYWIDGET_H
