#pragma once

#include <QComboBox>
/**
 * @brief Custom QComboBox created to prevent the menu popup from opening up at different
 *        offsets for different items, which may result in list items being rendered outside 
 *        of the screen/containing widget.
 */
class DirectionalComboBox : public QComboBox
{
    Q_OBJECT

public:
    explicit DirectionalComboBox(QWidget *parent = nullptr, bool upwards = true);

    void setPopupDirection(bool upwards);

private:
    bool popupUpwards;

    void showPopup();
};

