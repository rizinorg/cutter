#include "BoolToggleDelegate.h"
#include <QEvent>

BoolTogggleDelegate::BoolTogggleDelegate(QObject *parent)
    : QStyledItemDelegate(parent)
{
}

QWidget *BoolTogggleDelegate::createEditor(QWidget *parent,
                                           const QStyleOptionViewItem &option,
                                           const QModelIndex &index) const
{
    if (index.data(Qt::EditRole).type() == QVariant::Bool) {
        return nullptr;
    }
    return QStyledItemDelegate::createEditor(parent, option, index);
}

bool BoolTogggleDelegate::editorEvent(QEvent *event, QAbstractItemModel *model,
                                      const QStyleOptionViewItem &option, const QModelIndex &index)
{
    if (model->flags(index).testFlag(Qt::ItemFlag::ItemIsEditable)) {
        if (event->type() == QEvent::MouseButtonDblClick) {
            auto data = index.data(Qt::EditRole);
            if (data.type() == QVariant::Bool) {
                model->setData(index, !data.toBool());
                return true;
            }
        }
    }
    return QStyledItemDelegate::editorEvent(event, model, option, index);
}
