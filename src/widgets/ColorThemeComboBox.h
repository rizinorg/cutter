#ifndef COLORTHEMECOMBOBOX_H
#define COLORTHEMECOMBOBOX_H

#include <QComboBox>

/**
 * @brief The ColorThemeComboBox class provides combobox with Cutter color themes.
 */
class ColorThemeComboBox : public QComboBox
{
    Q_OBJECT

#if QT_VERSION < QT_VERSION_CHECK(5, 13, 0)
#    define Q_DISABLE_COPY(ColorThemeComboBox)                                                     \
        ColorThemeComboBox(const ColorThemeComboBox &m) = delete;                                  \
        ColorThemeComboBox &operator=(const ColorThemeComboBox &m) = delete;

#    define Q_DISABLE_MOVE(ColorThemeComboBox)                                                     \
        ColorThemeComboBox(ColorThemeComboBox &&m) = delete;                                       \
        ColorThemeComboBox &operator=(ColorThemeComboBox &&m) = delete;

#    define Q_DISABLE_COPY_MOVE(ColorThemeComboBox)                                                \
        Q_DISABLE_COPY(ColorThemeComboBox)                                                         \
        Q_DISABLE_MOVE(ColorThemeComboBox)
#endif

    Q_DISABLE_COPY_MOVE(ColorThemeComboBox)

public:
    explicit ColorThemeComboBox(QWidget *parent = nullptr);
    ~ColorThemeComboBox() override;

    /**
     * @brief setShowOnlyCustom sets whether or not combobox should contain only
     * custom themes (created by user or imported) or custom and srandard rizin themes.
     */
    void setShowOnlyCustom(bool value);

public slots:
    /**
     * @brief updateFromConfig updates list of themes to be shown.
     * @param interfaceThemeChanged should be set to true if the interface theme of Cutter was
     * changed since the last call to the function. This will preserve the selected item in the
     * combo box.
     *
     */
    void updateFromConfig(bool interfaceThemeChanged = false);

private slots:
    void onCurrentIndexChanged(int index);

private:
    bool showOnlyCustom;
};

#endif // COLORTHEMECOMBOBOX_H
