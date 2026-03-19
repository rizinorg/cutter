#ifndef ITEMCOUNTLINEEDIT_H
#define ITEMCOUNTLINEEDIT_H

#include <QLineEdit>

class QLabel;

/**
 * @brief A line edit containing an item count label on the right side
 */
class ItemCountLineEdit : public QLineEdit
{
    Q_OBJECT

public:
    explicit ItemCountLineEdit(QWidget *parent = nullptr);

    void setItemCount(int count);
    bool itemCountVisible() const;

    void setItemCountAutoHide(bool value);
    bool itemCountAutoHide() const;

public slots:
    void showItemCount(bool show);

protected:
    void resizeEvent(QResizeEvent *event) override;

private:
    QLabel *m_itemCountLabel;
    bool m_itemCountVisible;
    bool m_itemCountAutoHide;

    void updateLabelPosition();
};

#endif // ITEMCOUNTLINEEDIT_H
