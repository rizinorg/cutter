#ifndef CUTTERTREEVIEW_H
#define CUTTERTREEVIEW_H

#include "core/CutterCommon.h"

#include <QAbstractItemView>
#include <QTreeView>

#include <memory>

namespace Ui {
class CutterTreeView;
}

/**
 * @brief QTreeView wrapper for Cutter for default style and functionality
 */
class CUTTER_EXPORT CutterTreeView : public QTreeView
{
    Q_OBJECT

public:
    explicit CutterTreeView(QWidget *parent = nullptr);
    ~CutterTreeView();

    static void applyCutterStyle(QTreeView *view);

private:
    std::unique_ptr<Ui::CutterTreeView> ui;
};

#endif // CUTTERTREEVIEW_H
