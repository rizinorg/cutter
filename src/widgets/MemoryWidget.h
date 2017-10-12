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

    QTabWidget       *memTabWidget;
    QWebEngineView         *histoWebView;

    Highlighter        *highlighter;

signals:
    void fontChanged(QFont font);

public slots:
    void setMiniGraph(QString at);

    void switchTheme(bool dark);

protected:
    void resizeEvent(QResizeEvent *event) override;

private:
    std::unique_ptr<Ui::MemoryWidget> ui;
    CutterCore *core;

private slots:
    void on_seekChanged(RVA addr);

    void highlightPreviewCurrentLine();
    void highlightDecoCurrentLine();
    void setFonts(QFont font);

    void on_actionSettings_menu_1_triggered();
    void on_actionHideDisasm_side_panel_triggered();
    void on_actionHideGraph_side_panel_triggered();

    void on_disasButton_clicked();
    void on_hexButton_clicked();

    void updateViews(RVA offset = RVA_INVALID);
    void cycleViews();
    void on_hexSideTab_2_currentChanged(int index);
    void on_memSideToolButton_clicked();
    void on_previewToolButton_clicked();
    void on_decoToolButton_clicked();
    void on_previewToolButton_2_clicked();
    void on_simpleGrapgToolButton_clicked();
    void seek_back();
    void on_memTabWidget_currentChanged(int index);
};

#endif // MEMORYWIDGET_H
