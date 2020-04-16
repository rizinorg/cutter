#ifndef BOOLTOGGGLEDELEGATE_H
#define BOOLTOGGGLEDELEGATE_H

#include <QStyledItemDelegate>

class BoolTogggleDelegate : public QStyledItemDelegate
{
public:
    BoolTogggleDelegate(QObject *parent = nullptr);

    QWidget *createEditor(QWidget *parent,
                          const QStyleOptionViewItem &option,
                          const QModelIndex &index) const override;

    bool editorEvent(QEvent *event, QAbstractItemModel *model, const QStyleOptionViewItem &option,
                     const QModelIndex &index) override;
};


#endif // BOOLTOGGGLEDELEGATE_H
