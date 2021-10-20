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
    Q_DISABLE_COPY_MOVE(CutterTreeView)

public:
    explicit CutterTreeView(QWidget *parent = nullptr);
    ~CutterTreeView();

private:
    std::unique_ptr<Ui::CutterTreeView> ui;
};

#endif // CUTTERTREEVIEW_H
