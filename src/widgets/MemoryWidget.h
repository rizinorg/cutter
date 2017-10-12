#ifndef MEMORYWIDGET_H
#define MEMORYWIDGET_H

#include <QDebug>
#include <QTextEdit>
#include <QDockWidget>
#include <QTreeWidget>
#include <QTabWidget>
#include <QWebEngineView>
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
    class MemoryWidget;
}

class MemoryWidget : public DockWidget
{
    Q_OBJECT

public:
    explicit MemoryWidget();
    ~MemoryWidget();

    void setup() override;

    void refresh() override;

    QTextEdit        *hexOffsetText;
    QPlainTextEdit   *hexDisasTextEdit;
    QTextEdit        *hexASCIIText;
    QTextEdit        *hexHexText;
    QTreeWidget      *xrefToTreeWidget_2;
    QTreeWidget      *xreFromTreeWidget_2;
    QTabWidget       *memTabWidget;
    QWebEngineView         *histoWebView;

    Highlighter        *highlighter;
    Highlighter        *highlighter_5;
    AsciiHighlighter   *ascii_highlighter;
    HexHighlighter     *hex_highlighter;
    Highlighter        *preview_highlighter;
    Highlighter        *deco_highlighter;

signals:
    void fontChanged(QFont font);

public slots:
    void fillPlugins();

    void refreshHexdump(const QString &where = QString());

    QString normalize_addr(QString addr);

    QString normalizeAddr(QString addr);

    void setMiniGraph(QString at);

    void switchTheme(bool dark);

    void selectHexPreview();

    void showOffsets(bool show);

protected:
    void resizeEvent(QResizeEvent *event) override;

private:
    std::unique_ptr<Ui::MemoryWidget> ui;
    CutterCore *core;

    ut64 hexdumpTopOffset;
    ut64 hexdumpBottomOffset;
    QString last_fcn;

    RVA disasm_top_offset;
    RVA next_disasm_top_offset;

    RVA last_graph_fcn;
    RVA last_hexdump_fcn;

private slots:
    void on_seekChanged(RVA addr);

    void highlightHexCurrentLine();
    void highlightPreviewCurrentLine();
    void highlightDecoCurrentLine();
    void setFonts(QFont font);

    void highlightHexWords(const QString &str);
    void on_actionSettings_menu_1_triggered();
    void on_actionHideDisasm_side_panel_triggered();
    void on_actionHideHexdump_side_panel_triggered();
    void on_actionHideGraph_side_panel_triggered();

    void on_disasButton_clicked();
    void on_hexButton_clicked();
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

    void updateViews(RVA offset = RVA_INVALID);
    void cycleViews();
    void on_codeCombo_2_currentTextChanged(const QString &arg1);
    void on_hexSideTab_2_currentChanged(int index);
    void on_memSideToolButton_clicked();
    void on_previewToolButton_clicked();
    void on_decoToolButton_clicked();
    void on_previewToolButton_2_clicked();
    void on_copyMD5_clicked();
    void on_copySHA1_clicked();
    void on_simpleGrapgToolButton_clicked();
    void seek_back();
    void on_memTabWidget_currentChanged(int index);
};

#endif // MEMORYWIDGET_H
