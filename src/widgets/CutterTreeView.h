#ifndef CUTTERTREEVIEW_H
#define CUTTERTREEVIEW_H

#include <memory>
#include <QAbstractItemView>
#include <QTreeView>

namespace Ui {
class CutterTreeView;
}

class CutterTreeView : public QTreeView
{
    Q_OBJECT

public:
    explicit CutterTreeView(QWidget *parent = nullptr);
    ~CutterTreeView();

private:
    std::unique_ptr<Ui::CutterTreeView> ui;
};

#endif //CUTTERTREEVIEW_H
