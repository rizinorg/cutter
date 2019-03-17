
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
    void updateThemeFromConfig(bool qtThemeChanged = true);

    void on_fontSelectionButton_clicked();
    void on_themeComboBox_currentIndexChanged(int index);
    void on_colorComboBox_currentIndexChanged(int index);
    void on_copyButton_clicked();
    void on_deleteButton_clicked();

    /**
     * @brief Imports scheme file specified by user to custom schemes
     * directory.
     */
    void on_importButton_clicked();

    /**
     * @brief Exports current color scheme to file
     * specified by user.
     */
    void on_exportButton_clicked();

    /**
     * @brief Shows dialog to rename current color scheme.
     */
    void on_renameButton_clicked();
    void onLanguageComboBoxCurrentIndexChanged(int index);

};


#endif //ASMOPTIONSDIALOG_H
