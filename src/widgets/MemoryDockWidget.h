#ifndef MEMORYDOCKWIDGET_H
#define MEMORYDOCKWIDGET_H

#include "AddressableDockWidget.h"
#include "core/Cutter.h"

#include <QAction>

/* Disassembly/Graph/Hexdump/Decompiler view priority */
enum class MemoryWidgetType { Disassembly, Graph, Hexdump, Decompiler };

class MemoryDockWidget : public AddressableDockWidget
{
    Q_OBJECT
public:
    MemoryDockWidget(MemoryWidgetType type, MainWindow *parent);
    ~MemoryDockWidget() override {}

    bool tryRaiseMemoryWidget();
    MemoryWidgetType getType() const
    {
        return mType;
    }
    bool eventFilter(QObject *object, QEvent *event) override;
private:

    MemoryWidgetType mType;
};

#endif // MEMORYDOCKWIDGET_H
