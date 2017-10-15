
#ifndef SIDEBARWIDGET_H
#define SIDEBARWIDGET_H


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
    class SidebarWidget;
}

class SidebarWidget : public QDockWidget
{
Q_OBJECT

public:
    explicit SidebarWidget(const QString &title, QWidget *parent = nullptr, Qt::WindowFlags flags = 0);
    explicit SidebarWidget(QWidget *parent = nullptr, Qt::WindowFlags flags = 0);
    ~SidebarWidget();

    QTreeWidget      *xrefToTreeWidget_2;
    QTreeWidget      *xreFromTreeWidget_2;

private:
    std::unique_ptr<Ui::SidebarWidget> ui;
    CutterCore *core;

    void setFcnName(RVA addr);
    void get_refs_data(RVA addr);
    void fill_refs(QList<XrefDescription> refs, QList<XrefDescription> xrefs, QList<int> graph_data);
    void fillOffsetInfo(QString off);

    void setScrollMode();

private slots:
    void on_seekChanged(RVA addr);

    void refresh(RVA addr = RVA_INVALID);

    void on_xreFromTreeWidget_2_itemDoubleClicked(QTreeWidgetItem *item, int column);
    void on_xrefToTreeWidget_2_itemDoubleClicked(QTreeWidgetItem *item, int column);
    void on_xrefFromToolButton_2_clicked();
    void on_xrefToToolButton_2_clicked();
    void on_offsetToolButton_clicked();
};

#endif // SIDEBARWIDGET_H
