#ifndef BOOLTOGGLEDELEGATE_H
#define BOOLTOGGLEDELEGATE_H

#include "core/CutterCommon.h"

#include <QStyledItemDelegate>

/**
 * @brief Delegate that toggles boolean model values on double click instead of opening an
 * editor
 */
class CUTTER_EXPORT BoolToggleDelegate : public QStyledItemDelegate
{
public:
    BoolToggleDelegate(QObject *parent = nullptr);

    QWidget *createEditor(QWidget *parent, const QStyleOptionViewItem &option,
                          const QModelIndex &index) const override;

    bool editorEvent(QEvent *event, QAbstractItemModel *model, const QStyleOptionViewItem &option,
                     const QModelIndex &index) override;
};

#endif // BOOLTOGGLEDELEGATE_H
