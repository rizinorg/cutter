#ifndef MEMORYDOCKWIDGET_H
#define MEMORYDOCKWIDGET_H

#include "AddressableDockWidget.h"
#include "CutterCommon.h"

#include <QAction>

/* Disassembly/Graph/Hexdump/Decompiler view priority */
enum class MemoryWidgetType : ut8 {
    Disassembly,
    Graph,
    Hexdump,
    Decompiler,
    CallGraph,
    GlobalCallGraph
};

/**
 * @brief A base window that keeps memory-related views focused and synced
 */
class CUTTER_EXPORT MemoryDockWidget : public AddressableDockWidget
{
    Q_OBJECT
public:
    MemoryDockWidget(MemoryWidgetType type, MainWindow *parent);
    ~MemoryDockWidget() override {}

    bool tryRaiseMemoryWidget();
    MemoryWidgetType getType() const { return type; }
    bool eventFilter(QObject *object, QEvent *event) override;

private:
    MemoryWidgetType type;
};

#endif // MEMORYDOCKWIDGET_H
