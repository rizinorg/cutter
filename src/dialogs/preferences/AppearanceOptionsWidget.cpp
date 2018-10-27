#include <QDir>
#include <QLabel>
#include <QFontDialog>
#include <QTranslator>
#include <QInputDialog>

#include <QComboBox>
#include "PreferencesDialog.h"
#include "AppearanceOptionsWidget.h"
#include "ui_AppearanceOptionsWidget.h"

#include "common/Helpers.h"
#include "common/Configuration.h"

#include "common/ColorSchemeFileSaver.h"
#include "widgets/ColorSchemePrefWidget.h"

QStringList findLanguages()
{
    QDir dir(QCoreApplication::applicationDirPath() + QDir::separator() +
             "translations");
    QStringList fileNames = dir.entryList(QStringList("cutter_*.qm"), QDir::Files,
                                          QDir::Name);

    QStringList languages;
    QString currLanguageName;
    auto allLocales = QLocale::matchingLocales(QLocale::AnyLanguage, QLocale::AnyScript,
                                               QLocale::AnyCountry);

    for (auto i : fileNames) {
        QString localeName = i.mid(sizeof("cutter_") - 1, 2);
        for (auto j : allLocales) {
            if (j.name().startsWith(localeName)) {
                currLanguageName = j.nativeLanguageName();
                currLanguageName = currLanguageName.at(0).toUpper() +
                                   currLanguageName.right(currLanguageName.length() - 1);
                languages << currLanguageName;
                break;
            }
        }
    }

    return languages << "English";
}

AppearanceOptionsWidget::AppearanceOptionsWidget(PreferencesDialog *dialog, QWidget *parent)
    : QDialog(parent),
      ui(new Ui::AppearanceOptionsWidget)
{
    Q_UNUSED(dialog);
    ui->setupUi(this);

    updateFontFromConfig();
    updateThemeFromConfig();

    QStringList langs = findLanguages();
    ui->languageComboBox->addItems(langs);

    QString curr = Config()->getCurrLanguage();
    if (!langs.contains(curr))
        curr = "English";
    ui->languageComboBox->setCurrentText(curr);
    connect(ui->languageComboBox,
            static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
            this,
            &AppearanceOptionsWidget::onLanguageComboBoxCurrentIndexChanged);

    connect(Config(), &Configuration::fontsUpdated, this,
            &AppearanceOptionsWidget::updateFontFromConfig);
    connect(ui.get()->colorComboBox, &QComboBox::currentTextChanged, [&](const QString & name) {
        static_cast<ColorSchemePrefWidget *>(ui.get()->colorSchemePrefWidget)->setNewScheme(name);
    });
    static_cast<ColorSchemePrefWidget *>
    (ui.get()->colorSchemePrefWidget)->setNewScheme(Config()->getCurrentTheme());
}

AppearanceOptionsWidget::~AppearanceOptionsWidget() {}

void AppearanceOptionsWidget::updateFontFromConfig()
{
    QFont currentFont = Config()->getFont();
    ui->fontSelectionLabel->setText(currentFont.toString());
}

void AppearanceOptionsWidget::updateThemeFromConfig()
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

void AppearanceOptionsWidget::on_fontSelectionButton_clicked()
{
    QFont currentFont = Config()->getFont();
    bool ok;
    QFont newFont = QFontDialog::getFont(&ok, currentFont, this, QString(),
                                         QFontDialog::DontUseNativeDialog);
    if (ok) {
        Config()->setFont(newFont);
    }
}

void AppearanceOptionsWidget::on_themeComboBox_currentIndexChanged(int index)
{
    //disconnect(Config(), SIGNAL(colorsUpdated()), this, SLOT(updateThemeFromConfig()));
    Config()->setTheme(index);
    //connect(Config(), SIGNAL(colorsUpdated()), this, SLOT(updateThemeFromConfig()));
}

void AppearanceOptionsWidget::on_colorComboBox_currentIndexChanged(int index)
{
    QString theme = ui->colorComboBox->itemText(index);
    Config()->setColorTheme(theme);
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

void AppearanceOptionsWidget::onLanguageComboBoxCurrentIndexChanged(int index)
{
    QString language = ui->languageComboBox->itemText(index);
    Config()->setLanguage(language);

    QMessageBox mb;
    mb.setWindowTitle(tr("Language settings"));
    mb.setText(tr("Language will be changed after next application start."));
    mb.setIcon(QMessageBox::Information);
    mb.setStandardButtons(QMessageBox::Ok);
    mb.exec();
}
