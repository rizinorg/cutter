#include "BoolToggleDelegate.h"

#include <QEvent>

BoolToggleDelegate::BoolToggleDelegate(QObject *parent) : QStyledItemDelegate(parent) {}

QWidget *BoolToggleDelegate::createEditor(QWidget *parent, const QStyleOptionViewItem &option,
                                          const QModelIndex &index) const
{
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    if (index.data(Qt::EditRole).typeId() == QMetaType::Bool) {
#else
    if (index.data(Qt::EditRole).type() == QVariant::Bool) {
#endif
        return nullptr;
    }
    return QStyledItemDelegate::createEditor(parent, option, index);
}

bool BoolToggleDelegate::editorEvent(QEvent *event, QAbstractItemModel *model,
                                     const QStyleOptionViewItem &option, const QModelIndex &index)
{
    if (model->flags(index).testFlag(Qt::ItemFlag::ItemIsEditable)) {
        if (event->type() == QEvent::MouseButtonDblClick) {
            auto data = index.data(Qt::EditRole);
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
            if (data.typeId() == QMetaType::Bool) {
#else
            if (data.type() == QVariant::Bool) {
#endif
                model->setData(index, !data.toBool());
                return true;
            }
        }
    }
    return QStyledItemDelegate::editorEvent(event, model, option, index);
}
