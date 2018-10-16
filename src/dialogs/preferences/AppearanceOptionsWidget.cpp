#include <QDir>
#include <QLabel>
#include <QFontDialog>
#include <QInputDialog>

#include <QComboBox>
#include "PreferencesDialog.h"
#include "AppearanceOptionsWidget.h"
#include "ui_AppearanceOptionsWidget.h"

#include "utils/Helpers.h"
#include "utils/Configuration.h"

#include "utils/ColorSchemeFileSaver.h"
#include "widgets/ColorSchemePrefWidget.h"

AppearanceOptionsWidget::AppearanceOptionsWidget(PreferencesDialog *dialog, QWidget *parent)
    : AbstractOptionWidget(parent),
      ui(new Ui::AppearanceOptionsWidget)
{
    Q_UNUSED(dialog);
    ui->setupUi(this);

    updateFontFromConfig();
    updateThemeFromConfig();

    connect(Config(), &Configuration::fontsUpdated, this,
            &AppearanceOptionsWidget::updateFontFromConfig);
    connect(ui->colorComboBox, &QComboBox::currentTextChanged, [&](const QString & name) {
        static_cast<ColorSchemePrefWidget *>(ui->colorSchemePrefWidget)->setNewScheme(name);
    });
    connect(ui->colorSchemePrefWidget, &ColorSchemePrefWidget::colorChanged, [&]() {
        isChanged = true;
    });
    static_cast<ColorSchemePrefWidget *>
    (ui->colorSchemePrefWidget)->setNewScheme(Config()->getCurrentTheme());
}

AppearanceOptionsWidget::~AppearanceOptionsWidget() {}

void AppearanceOptionsWidget::apply()
{
    Config()->setFont(currSettings.value("font").value<QFont>());
    Config()->setTheme(currSettings.value("Qt theme").toInt());
    static_cast<ColorSchemePrefWidget *>(ui->colorSchemePrefWidget)->apply();
    Config()->setColorTheme(currSettings.value("color theme").toString());
    isChanged = false;
}

void AppearanceOptionsWidget::discard()
{
    updateFontFromConfig();
    updateThemeFromConfig();
    static_cast<ColorSchemePrefWidget *>
    (ui->colorSchemePrefWidget)->setNewScheme(Config()->getCurrentTheme());
    isChanged = false;
}

QString AppearanceOptionsWidget::getChoosenTheme() const
{
    return ui->colorComboBox->currentText();
}

void AppearanceOptionsWidget::updateFontFromConfig()
{
    QFont currentFont = Config()->getFont();
    currSettings.setValue("font", currentFont);
    ui->fontSelectionLabel->setText(currentFont.toString());
}

void AppearanceOptionsWidget::updateThemeFromConfig()
{
    // Disconnect currentIndexChanged because clearing the comboxBox and refiling it causes its index to change.
    disconnect(ui->colorComboBox, SIGNAL(currentIndexChanged(int)), this,
               SLOT(on_colorComboBox_currentIndexChanged(int)));
    ui->themeComboBox->setCurrentIndex(Config()->getTheme());
    currSettings.setValue("Qt theme", Config()->getTheme());

    QList<QString> themes = Core()->getColorThemes();
    ui->colorComboBox->clear();
    ui->colorComboBox->addItem("default");
    for (QString str : themes)
        ui->colorComboBox->addItem(str);
    QString curTheme = Config()->getCurrentTheme();
    currSettings.setValue("color theme", curTheme);
    int index = themes.indexOf(curTheme) + 1;
    ui->colorComboBox->setCurrentIndex(index);
    int maxThemeLen = 0;
    for (QString str : themes) {
        int strLen = str.length();
        if (strLen > maxThemeLen) {
            maxThemeLen = strLen;
        }
    }
    ui->colorComboBox->setMinimumContentsLength(maxThemeLen);
    ui->colorComboBox->setSizeAdjustPolicy(QComboBox::AdjustToMinimumContentsLength);
    connect(ui->colorComboBox, SIGNAL(currentIndexChanged(int)), this,
            SLOT(on_colorComboBox_currentIndexChanged(int)));
}

void AppearanceOptionsWidget::on_fontSelectionButton_clicked()
{
    QFont currentFont = Config()->getFont();
    bool ok;
    QFont newFont = QFontDialog::getFont(&ok, currentFont, this, QString(),
                                         QFontDialog::DontUseNativeDialog);
    if (ok) {
        currSettings.setValue("font", newFont);
        isChanged = true;
    }
}

void AppearanceOptionsWidget::on_themeComboBox_currentIndexChanged(int index)
{
    //disconnect(Config(), SIGNAL(colorsUpdated()), this, SLOT(updateThemeFromConfig()));
    currSettings.setValue("Qt theme", index);
    isChanged = true;
    //connect(Config(), SIGNAL(colorsUpdated()), this, SLOT(updateThemeFromConfig()));
}

void AppearanceOptionsWidget::on_colorComboBox_currentIndexChanged(int index)
{
    QString theme = ui->colorComboBox->itemText(index);
    currSettings.setValue("color theme", ui->colorComboBox->currentText());
    isChanged = true;
}

void AppearanceOptionsWidget::on_copyButton_clicked()
{
    QString newSchemeName;
    do {
        newSchemeName = QInputDialog::getText(this, tr("Enter scheme name"),
                                              tr("Name:"), QLineEdit::Normal,
                                              QDir::home().dirName());
    } while (ColorSchemeFileWorker().isNameEngaged(newSchemeName) && !newSchemeName.isEmpty());

    if (newSchemeName.isEmpty())
        return;
    ColorSchemeFileWorker().copy(Config()->getCurrentTheme(), newSchemeName);
    Config()->setColorTheme(newSchemeName);
    ui.get()->colorSchemePrefWidget->setNewScheme(newSchemeName);
    updateThemeFromConfig();
    ui.get()->colorComboBox->setCurrentIndex(ui.get()->colorComboBox->findText(newSchemeName));
}

void AppearanceOptionsWidget::on_deleteButton_clicked()
{
    ColorSchemeFileWorker().deleteScheme(Config()->getCurrentTheme());
}
