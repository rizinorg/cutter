
#ifndef SIDEBARWIDGET_H
#define SIDEBARWIDGET_H


#include <QDebug>
#include <QTextEdit>
#include <QTreeWidget>
#include <QTabWidget>
#include <QUrl>
#include <QPlainTextEdit>
#include <QMouseEvent>
#include <memory>

#include "Cutter.h"
#include "CutterDockWidget.h"
#include "utils/Highlighter.h"
#include "utils/HexAsciiHighlighter.h"
#include "utils/HexHighlighter.h"
#include "Dashboard.h"


namespace Ui {
class SidebarWidget;
}

class SidebarWidget : public CutterDockWidget
{
    Q_OBJECT

public:
    explicit SidebarWidget(MainWindow *main, QAction *action = nullptr);
    ~SidebarWidget();

private:
    std::unique_ptr<Ui::SidebarWidget> ui;

    void setFcnName(RVA addr);
    void updateRefs(RVA addr);
    void fillRefs(QList<XrefDescription> refs, QList<XrefDescription> xrefs);
    void fillOffsetInfo(QString off);
    void fillRegistersInfo();

    void setScrollMode();

private slots:
    void on_seekChanged(RVA addr);

    void refresh(RVA addr = RVA_INVALID);

    void on_xrefFromTreeWidget_itemDoubleClicked(QTreeWidgetItem *item, int column);
    void on_xrefToTreeWidget_itemDoubleClicked(QTreeWidgetItem *item, int column);

    void on_offsetToolButton_clicked();
    void on_opcodeDescToolButton_clicked();
    void on_xrefFromToolButton_clicked();
    void on_xrefToToolButton_clicked();
    void on_regInfoToolButton_clicked();
};

#endif // SIDEBARWIDGET_H
