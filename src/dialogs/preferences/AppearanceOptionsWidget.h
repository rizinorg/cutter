
#ifndef APPEARANCEOPTIONSWIDGET_H
#define APPEARANCEOPTIONSWIDGET_H

#include <QDialog>
#include <QSettings>
#include <QPushButton>
#include <memory>

#include "Cutter.h"
#include "AbstractOptionWidget.h"

class PreferencesDialog;

namespace Ui {
class AppearanceOptionsWidget;
}

class AppearanceOptionsWidget : public AbstractOptionWidget
{
    Q_OBJECT

public:
    explicit AppearanceOptionsWidget(PreferencesDialog *dialog, QWidget *parent = nullptr);
    ~AppearanceOptionsWidget();

    void apply();
    void discard();

    QString getChoosenTheme() const;

private:
    std::unique_ptr<Ui::AppearanceOptionsWidget> ui;

private slots:
    void updateFontFromConfig();
    void updateThemeFromConfig();

    void on_fontSelectionButton_clicked();
    void on_themeComboBox_currentIndexChanged(int index);
    void on_colorComboBox_currentIndexChanged(int index);
    void on_copyButton_clicked();
    void on_deleteButton_clicked();
};


#endif //ASMOPTIONSDIALOG_H
