#include <QDir>
#include <QLabel>
#include <QFontDialog>
#include <QInputDialog>

#include <QComboBox>
#include "PreferencesDialog.h"
#include "GeneralOptionsWidget.h"
#include "ui_GeneralOptionsWidget.h"

#include "utils/Helpers.h"
#include "utils/Configuration.h"

#include "utils/ColorSchemeFileSaver.h"
#include "widgets/ColorSchemePrefWidget.h"

GeneralOptionsWidget::GeneralOptionsWidget(PreferencesDialog *dialog, QWidget *parent)
    : QDialog(parent),
      ui(new Ui::GeneralOptionsWidget)
{
    Q_UNUSED(dialog);
    ui->setupUi(this);

    updateFontFromConfig();
    updateThemeFromConfig();

    connect(Config(), &Configuration::fontsUpdated, this, &GeneralOptionsWidget::updateFontFromConfig);
    connect(ui.get()->colorComboBox, &QComboBox::currentTextChanged, [&](const QString & name) {
        static_cast<ColorSchemePrefWidget *>(ui.get()->colorSchemePrefWidget)->setNewScheme(name);
    });
    static_cast<ColorSchemePrefWidget *>
    (ui.get()->colorSchemePrefWidget)->setNewScheme(Config()->getCurrentTheme());
}

GeneralOptionsWidget::~GeneralOptionsWidget() {}

void GeneralOptionsWidget::updateFontFromConfig()
{
    QFont currentFont = Config()->getFont();
    ui->fontSelectionLabel->setText(currentFont.toString());
}

void GeneralOptionsWidget::updateThemeFromConfig()
{
    // Disconnect currentIndexChanged because clearing the comboxBox and refiling it causes its index to change.
    disconnect(ui->colorComboBox, SIGNAL(currentIndexChanged(int)), this,
               SLOT(on_colorComboBox_currentIndexChanged(int)));
    ui->themeComboBox->setCurrentIndex(Config()->getTheme());

    QList<QString> themes = Core()->getColorThemes();
    ui->colorComboBox->clear();
    ui->colorComboBox->addItem("default");
    for (QString str : themes)
        ui->colorComboBox->addItem(str);
    QString curTheme = Config()->getCurrentTheme();
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

void GeneralOptionsWidget::on_fontSelectionButton_clicked()
{
    QFont currentFont = Config()->getFont();
    bool ok;
    QFont newFont = QFontDialog::getFont(&ok, currentFont, this, QString(),
                                         QFontDialog::DontUseNativeDialog);
    if (ok) {
        Config()->setFont(newFont);
    }
}

void GeneralOptionsWidget::on_themeComboBox_currentIndexChanged(int index)
{
    //disconnect(Config(), SIGNAL(colorsUpdated()), this, SLOT(updateThemeFromConfig()));
    Config()->setTheme(index);
    //connect(Config(), SIGNAL(colorsUpdated()), this, SLOT(updateThemeFromConfig()));
}

void GeneralOptionsWidget::on_colorComboBox_currentIndexChanged(int index)
{
    QString theme = ui->colorComboBox->itemText(index);
    Config()->setColorTheme(theme);
}

void GeneralOptionsWidget::on_copyButton_clicked()
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

void GeneralOptionsWidget::on_deleteButton_clicked()
{
    ColorSchemeFileWorker().deleteScheme(Config()->getCurrentTheme());
}
