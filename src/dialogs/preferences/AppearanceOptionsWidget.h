
#ifndef AppearanceOptionsWidget_H
#define AppearanceOptionsWidget_H

#include <QDialog>
#include <QPushButton>
#include <memory>

#include "Cutter.h"

class PreferencesDialog;

namespace Ui {
class AppearanceOptionsWidget;
}

class AppearanceOptionsWidget : public QDialog
{
    Q_OBJECT

public:
    explicit AppearanceOptionsWidget(PreferencesDialog *dialog, QWidget *parent = nullptr);
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
    void onLanguageComboBoxCurrentIndexChanged(int index);
};


#endif //ASMOPTIONSDIALOG_H
