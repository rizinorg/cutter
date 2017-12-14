#include <QLabel>
#include <QFontDialog>

#include "GeneralOptionsWidget.h"
#include "ui_GeneralOptionsWidget.h"

#include "PreferencesDialog.h"

#include "utils/Helpers.h"
#include "utils/Configuration.h"

GeneralOptionsWidget::GeneralOptionsWidget(PreferencesDialog */*dialog*/, QWidget *parent)
  : QDialog(parent),
    ui(new Ui::GeneralOptionsWidget)
{
    ui->setupUi(this);

    updateFontFromConfig();
    updateThemeFromConfig();

    connect(Config(), SIGNAL(fontsUpdated()), this, SLOT(updateFontFromConfig()));
    connect(Config(), SIGNAL(colorsUpdated()), this, SLOT(updateThemeFromConfig()));
}

GeneralOptionsWidget::~GeneralOptionsWidget() {}

void GeneralOptionsWidget::updateFontFromConfig()
{
    QFont currentFont = Config()->getFont();
    ui->fontSelectionLabel->setText(currentFont.toString());
}

void GeneralOptionsWidget::updateThemeFromConfig()
{
    ui->themeComboBox->setCurrentIndex(Config()->getDarkTheme() ? 1 : 0);
}

void GeneralOptionsWidget::on_fontSelectionButton_clicked()
{
    QFont currentFont = Config()->getFont();
    bool ok;
    QFont newFont = QFontDialog::getFont(&ok, currentFont, this);
    if (ok) {
        Config()->setFont(newFont);
    }
}

void GeneralOptionsWidget::on_themeComboBox_currentIndexChanged(int index)
{
    disconnect(Config(), SIGNAL(colorsUpdated()), this, SLOT(updateThemeFromConfig()));
    Config()->setDarkTheme(index == 1);
    connect(Config(), SIGNAL(colorsUpdated()), this, SLOT(updateThemeFromConfig()));
}
