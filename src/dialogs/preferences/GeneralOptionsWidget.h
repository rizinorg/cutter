
#ifndef GENERALOPTIONSWIDGET_H
#define GENERALOPTIONSWIDGET_H

#include <QDialog>
#include <QPushButton>
#include <memory>

#include "Cutter.h"

class PreferencesDialog;

namespace Ui {
class GeneralOptionsWidget;
}

class GeneralOptionsWidget : public QDialog
{
    Q_OBJECT

public:
    explicit GeneralOptionsWidget(PreferencesDialog *dialog, QWidget *parent = nullptr);
    ~GeneralOptionsWidget();

private:
    std::unique_ptr<Ui::GeneralOptionsWidget> ui;

private slots:
    void updateFontFromConfig();
    void updateThemeFromConfig();

    void on_fontSelectionButton_clicked();
    void on_themeComboBox_currentIndexChanged(int index);
    void on_colorComboBox_currentIndexChanged(int index);
};


#endif //ASMOPTIONSDIALOG_H
