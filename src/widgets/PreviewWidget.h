#ifndef PREVIEWWIDGET_H
#define PREVIEWWIDGET_H

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
    class PreviewWidget;
}

class PreviewWidget : public QDockWidget
{
    Q_OBJECT

public:
    explicit PreviewWidget(const QString &title, QWidget *parent = nullptr, Qt::WindowFlags flags = 0);
    explicit PreviewWidget(QWidget *parent = nullptr, Qt::WindowFlags flags = 0);
    ~PreviewWidget();

    Highlighter        *highlighter;

signals:
    void fontChanged(QFont font);

public slots:
    void setMiniGraph(QString at);

    void switchTheme(bool dark);

private:
    std::unique_ptr<Ui::PreviewWidget> ui;
    CutterCore *core;

    void refresh(RVA addr = RVA_INVALID);

private slots:
    void on_seekChanged(RVA addr);

    void highlightPreviewCurrentLine();
    void highlightDecoCurrentLine();
    void setFonts(QFont font);

    void on_actionSettings_menu_1_triggered();
    void on_previewToolButton_clicked();
    void on_decoToolButton_clicked();
    void on_simpleGrapgToolButton_clicked();
};

#endif // MEMORYWIDGET_H
