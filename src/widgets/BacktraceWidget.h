#pragma once

#include <QJsonObject>
#include <memory>
#include <QStandardItem>
#include <QTableView>

#include "core/Cutter.h"
#include "CutterDockWidget.h"

class MainWindow;

namespace Ui {
class BacktraceWidget;
}

class BacktraceWidget : public CutterDockWidget
{
    Q_OBJECT

#if QT_VERSION < QT_VERSION_CHECK(5, 13, 0)
#    define Q_DISABLE_COPY(BacktraceWidget)                                                        \
        BacktraceWidget(const BacktraceWidget &m) = delete;                                        \
        BacktraceWidget &operator=(const BacktraceWidget &m) = delete;

#    define Q_DISABLE_MOVE(BacktraceWidget)                                                        \
        BacktraceWidget(BacktraceWidget &&m) = delete;                                             \
        BacktraceWidget &operator=(BacktraceWidget &&m) = delete;

#    define Q_DISABLE_COPY_MOVE(BacktraceWidget)                                                   \
        Q_DISABLE_COPY(BacktraceWidget)                                                            \
        Q_DISABLE_MOVE(BacktraceWidget)
#endif

    Q_DISABLE_COPY_MOVE(BacktraceWidget)

public:
    explicit BacktraceWidget(MainWindow *main);
    ~BacktraceWidget() override;

private slots:
    void updateContents();
    void setBacktraceGrid();
    void fontsUpdatedSlot();

private:
    std::unique_ptr<Ui::BacktraceWidget> ui;
    QStandardItemModel *modelBacktrace = new QStandardItemModel(1, 5, this);
    QTableView *viewBacktrace = new QTableView(this);
    RefreshDeferrer *refreshDeferrer;
};
