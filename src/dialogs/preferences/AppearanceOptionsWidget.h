
#ifndef AppearanceOptionsWidget_H
#define AppearanceOptionsWidget_H

#include <QDialog>
#include <QPushButton>
#include <memory>

#include "core/Cutter.h"

class PreferencesDialog;

namespace Ui {
class AppearanceOptionsWidget;
}

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

    void on_fontSelectionButton_clicked();
    void onFontZoomBoxValueChanged(int zoom);
    void on_themeComboBox_currentIndexChanged(int index);
    void on_copyButton_clicked();
    void on_deleteButton_clicked();

    /**
     * @brief Imports theme file specified by user to custom themes
     * directory.
     */
    void on_importButton_clicked();

    /**
     * @brief Exports current color theme to file
     * specified by user.
     */
    void on_exportButton_clicked();

    /**
     * @brief Shows dialog to rename current color theme.
     */
    void on_renameButton_clicked();
    void on_editButton_clicked();
    void onLanguageComboBoxCurrentIndexChanged(int index);

private:
    void updateModificationButtons(const QString& theme);
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
    QIcon getIconFromSvg(const QString &fileName, const QColor &after, const QColor &before = QColor());

};


#endif //ASMOPTIONSDIALOG_H
