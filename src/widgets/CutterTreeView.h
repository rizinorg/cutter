#ifndef CUTTERTREEVIEW_H
#define CUTTERTREEVIEW_H

#include "core/CutterCommon.h"

#include <memory>
#include <QAbstractItemView>
#include <QTreeView>

namespace Ui {
class CutterTreeView;
}

class CUTTER_EXPORT CutterTreeView : public QTreeView
{
    Q_OBJECT

#if QT_VERSION < QT_VERSION_CHECK(5, 13, 0)
#    define Q_DISABLE_COPY(CutterTreeView)                                                         \
        CutterTreeView(const CutterTreeView &w) = delete;                                          \
        CutterTreeView &operator=(const CutterTreeView &w) = delete;

#    define Q_DISABLE_MOVE(CutterTreeView)                                                         \
        CutterTreeView(CutterTreeView &&w) = delete;                                               \
        CutterTreeView &operator=(CutterTreeView &&w) = delete;

#    define Q_DISABLE_COPY_MOVE(CutterTreeView)                                                    \
        Q_DISABLE_COPY(CutterTreeView)                                                             \
        Q_DISABLE_MOVE(CutterTreeView)
#endif

    Q_DISABLE_COPY_MOVE(CutterTreeView)

public:
    explicit CutterTreeView(QWidget *parent = nullptr);
    ~CutterTreeView() override;

private:
    std::unique_ptr<Ui::CutterTreeView> ui;
};

#endif // CUTTERTREEVIEW_H
