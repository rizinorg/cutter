#ifndef COLORTHEMECOMBOBOX_H
#define COLORTHEMECOMBOBOX_H

#include <QComboBox>

/**
 * @brief The ColorThemeComboBox class provides combobox with Cutter color themes.
 */
class ColorThemeComboBox : public QComboBox
{
    Q_OBJECT
public:
    explicit ColorThemeComboBox(QWidget *parent = nullptr);

    /**
     * @brief setShowOnlyCustom sets whether or not combobox should contain only
     * custom themes (created by user or imported) or custom and srandard radare2 themes.
     */
    void setShowOnlyCustom(bool value);

public slots:
    /**
    * @brief updateFromConfig updates list of themes to be shown.
    * @param interfaceThemeChanged should be set to true if the interface theme of Cutter was changed
    * since the last call to the function. This will preserve the selected item in the combo box.
    *
    */
    void updateFromConfig(bool interfaceThemeChanged = false);

private slots:
    void onCurrentIndexChanged(int index);

private:
    bool showOnlyCustom;
};

#endif // COLORTHEMECOMBOBOX_H
