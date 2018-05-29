#pragma once

#include <QDebug>
#include <QTextEdit>
#include <QTreeWidget>
#include <QTabWidget>
#include <QUrl>
#include <QPlainTextEdit>
#include <QMouseEvent>
#include <QGridLayout>
#include <QJsonObject>
#include <memory>
#include <QStandardItem>
#include <QTableView>

#include "Cutter.h"
#include "CutterDockWidget.h"
#include "utils/Highlighter.h"
#include "utils/HexAsciiHighlighter.h"
#include "utils/HexHighlighter.h"
#include "Dashboard.h"
#include "CutterDockWidget.h"

class MainWindow;

namespace Ui {
class DebugWidget;
}

class DebugWidget : public CutterDockWidget
{
    Q_OBJECT

public:
    explicit DebugWidget(MainWindow *main, QAction *action = nullptr);
    ~DebugWidget();

private slots:
    void updateContents();
    void setRegisterGrid(int numcols);
    void setStackGrid();

private:
    std::unique_ptr<Ui::DebugWidget> ui;
    QStandardItemModel *model = new QStandardItemModel(8,3,this);
    QTableView *view = new QTableView;
    QGridLayout *registerLayout = new QGridLayout;
};