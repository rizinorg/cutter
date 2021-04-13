#include "ColorThemeComboBox.h"

#include "core/Cutter.h"

#include "common/ColorThemeWorker.h"
#include "common/Configuration.h"

ColorThemeComboBox::ColorThemeComboBox(QWidget *parent) : QComboBox(parent), showOnlyCustom(false)
{
    connect(this, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged), this,
            &ColorThemeComboBox::onCurrentIndexChanged);
    updateFromConfig();
}

void ColorThemeComboBox::updateFromConfig(bool interfaceThemeChanged)
{
    QSignalBlocker signalBlockerColorBox(this);

    const int curInterfaceThemeIndex = Config()->getInterfaceTheme();
    const QList<QString> themes(showOnlyCustom ? ThemeWorker().customThemes()
                                               : Core()->getColorThemes());

    clear();
    for (const QString &theme : themes) {
        if (ThemeWorker().isCustomTheme(theme)
            || (Configuration::cutterInterfaceThemesList()[curInterfaceThemeIndex].flag
                & Configuration::relevantThemes[theme])) {
            addItem(theme);
        }
    }

    QString curTheme = interfaceThemeChanged ? Config()->getLastThemeOf(
                               Configuration::cutterInterfaceThemesList()[curInterfaceThemeIndex])
                                             : Config()->getColorTheme();
    const int index = findText(curTheme);

    setCurrentIndex(index == -1 ? 0 : index);
    if (interfaceThemeChanged || index == -1) {
        curTheme = currentText();
        Config()->setColorTheme(curTheme);
    }
    int maxThemeLen = 0;
    for (const QString &str : themes) {
        int strLen = str.length();
        if (strLen > maxThemeLen) {
            maxThemeLen = strLen;
        }
    }
    setMinimumContentsLength(maxThemeLen);
    setSizeAdjustPolicy(QComboBox::AdjustToContents);
}

void ColorThemeComboBox::onCurrentIndexChanged(int index)
{
    QString theme = itemText(index);

    int curQtThemeIndex = Config()->getInterfaceTheme();
    if (curQtThemeIndex >= Configuration::cutterInterfaceThemesList().size()) {
        curQtThemeIndex = 0;
        Config()->setInterfaceTheme(curQtThemeIndex);
    }

    Config()->setLastThemeOf(Configuration::cutterInterfaceThemesList()[curQtThemeIndex], theme);
    Config()->setColorTheme(theme);
}

void ColorThemeComboBox::setShowOnlyCustom(bool value)
{
    showOnlyCustom = value;
    updateFromConfig();
}
