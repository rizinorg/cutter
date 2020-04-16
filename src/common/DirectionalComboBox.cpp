#include "DirectionalComboBox.h"

DirectionalComboBox::DirectionalComboBox(QWidget *parent, bool upwards)
    : QComboBox(parent), popupUpwards(upwards)
{
}

void DirectionalComboBox::showPopup()
{
    QComboBox::showPopup();
    QWidget *popup = this->findChild<QFrame *>();
    if (popupUpwards) {
        popup->move(popup->x(),
                    mapToGlobal(this->rect().bottomLeft()).y() - popup->height());
    } else {
        popup->move(popup->x(),
                    mapToGlobal(this->rect().topLeft()).y());
    }
}

void DirectionalComboBox::setPopupDirection(bool upwards)
{
    popupUpwards = upwards;
}
