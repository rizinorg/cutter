#ifndef AppearanceOptionsWidget_H
#define AppearanceOptionsWidget_H

#include "core/Cutter.h"

#include <QDialog>
#include <QPushButton>

#include <memory>

class PreferencesDialog;

namespace Ui {
class AppearanceOptionsWidget;
}

/**
 * @brief Contains configurable options related to appearance
 */
class AppearanceOptionsWidget : public QDialog
{
    Q_OBJECT

public:
    explicit AppearanceOptionsWidget(PreferencesDialog *dialog);
    ~AppearanceOptionsWidget();

private:
    std::unique_ptr<Ui::AppearanceOptionsWidget> ui;

private slots:
    void updateFontFromConfig();
    void updateThemeFromConfig(bool interfaceThemeChanged = true);

    void onFontSelectionButtonClicked();
    void onFontZoomBoxValueChanged(int zoom);
    void onThemeComboBoxCurrentIndexChanged(int index);
    void onCopyButtonClicked();
    void onDeleteButtonClicked();

    /**
     * @brief Imports theme file specified by user to custom themes
     * directory.
     */
    void onImportButtonClicked();

    /**
     * @brief Exports current color theme to file
     * specified by user.
     */
    void onExportButtonClicked();

    /**
     * @brief Shows dialog to rename current color theme.
     */
    void onRenameButtonClicked();
    void onEditButtonClicked();
    void onLanguageComboBoxCurrentIndexChanged(int index);

private:
    void updateModificationButtons(const QString &theme);
    void updateFromConfig();

    /**
     * @brief Changes all @a before colors in given @a fileName svg file to @a after
     * and returns result icon. If @a before is not specified, changes all colors.
     * @param fileName
     * Path to svg file.
     * @param after
     * What color should be inserted instead of old one.
     * @param before
     * Color that should be repalced.
     */
    QIcon getIconFromSvg(const QString &fileName, const QColor &after,
                         const QColor &before = QColor());
};

#endif // ASMOPTIONSDIALOG_H
