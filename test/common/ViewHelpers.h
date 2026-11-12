#ifndef CUTTER_TEST_VIEW_HELPERS_H
#define CUTTER_TEST_VIEW_HELPERS_H

#include "common/AddressableItemModel.h"
#include "common/CutterSeekable.h"
#include "core/MainWindow.h"
#include "widgets/DisassemblerGraphView.h"
#include "widgets/DisassemblyWidget.h"
#include "widgets/GraphWidget.h"

#include <QAbstractItemModel>
#include <QAbstractItemView>
#include <QDeadlineTimer>
#include <QList>
#include <QSet>
#include <QTest>

namespace CutterTest {

// TODO: document all of these

template<class Dock>
Dock *findSynchronizedDock(MainWindow *main)
{
    const QList<Dock *> docks = main->findChildren<Dock *>();
    for (Dock *dock : docks) {
        if (dock->getSeekable() && dock->getSeekable()->isSynchronized()) {
            return dock;
        }
    }
    return nullptr;
}

inline QSet<RVA> disassemblyOffsets(DisassemblyWidget *disassembly)
{
    QSet<RVA> offsets;
    const QList<DisassemblyLine> lines = disassembly->getLines();
    for (const DisassemblyLine &line : lines) {
        offsets.insert(line.offset);
    }
    return offsets;
}

inline bool disassemblyContains(DisassemblyWidget *disassembly, const QString &text)
{
    const QList<DisassemblyLine> lines = disassembly->getLines();
    for (const DisassemblyLine &line : lines) {
        if (line.text.contains(text)) {
            return true;
        }
    }
    return false;
}

inline RVA disassemblyTopOffset(DisassemblyWidget *disassembly)
{
    const QList<DisassemblyLine> lines = disassembly->getLines();
    const int start = disassembly->getStartIndex();
    return start >= 0 && start < lines.size() ? lines.at(start).offset : RVA_INVALID;
}

inline bool graphHasBlock(GraphWidget *graph, RVA address)
{
    return graph->getGraphView()->getBlocks().count(address) > 0;
}

inline bool modelContains(const QAbstractItemModel *model, const QString &text,
                          const QModelIndex &parent = QModelIndex())
{
    for (int row = 0; row < model->rowCount(parent); ++row) {
        for (int column = 0; column < model->columnCount(parent); ++column) {
            const QModelIndex index = model->index(row, column, parent);
            if (index.data(Qt::DisplayRole).toString().contains(text)) {
                return true;
            }
        }
        const QModelIndex child = model->index(row, 0, parent);
        if (model->hasChildren(child) && modelContains(model, text, child)) {
            return true;
        }
    }
    return false;
}

inline QAbstractItemView *addressableView(QWidget *parent)
{
    const QList<QAbstractItemView *> views = parent->findChildren<QAbstractItemView *>();
    for (QAbstractItemView *view : views) {
        if (view->model() && dynamic_cast<AddressableItemModelI *>(view->model())) {
            return view;
        }
    }
    return nullptr;
}

inline bool waitForRows(const QAbstractItemModel *model, int timeoutMs)
{
    const QDeadlineTimer deadline(timeoutMs);
    while (model->rowCount() == 0 && !deadline.hasExpired()) {
        QTest::qWait(20);
    }
    return model->rowCount() > 0;
}

} // namespace CutterTest

#endif // CUTTER_TEST_VIEW_HELPERS_H
