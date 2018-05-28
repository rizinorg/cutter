#pragma once

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

private:
    std::unique_ptr<Ui::DebugWidget> ui;
};