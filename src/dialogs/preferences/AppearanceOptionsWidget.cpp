#include <QDir>
#include <QLabel>
#include <QFontDialog>
#include <QTranslator>
#include <QInputDialog>
#include <QSignalBlocker>

#include <QComboBox>
#include "PreferencesDialog.h"
#include "AppearanceOptionsWidget.h"
#include "ui_AppearanceOptionsWidget.h"

#include "common/Helpers.h"
#include "common/Configuration.h"

#include "common/ColorSchemeFileSaver.h"
#include "widgets/ColorSchemePrefWidget.h"

static const QHash<QString, ColorFlags> kRelevantSchemes = {
    { "ayu", DarkFlag },
    { "consonance", DarkFlag },
    { "darkda", DarkFlag },
    { "onedark", DarkFlag },
    { "solarized", DarkFlag },
    { "zenburn", DarkFlag },
    { "cutter", LightFlag },
    { "dark", LightFlag },
    { "matrix", LightFlag },
    { "tango", LightFlag },
    { "white", LightFlag }
};

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

    QString curr = Config()->getCurrLocale().nativeLanguageName();
    curr = curr.at(0).toUpper() + curr.right(curr.length() - 1);
    if (!langs.contains(curr)) {
        curr = "English";
    }
    ui->languageComboBox->setCurrentText(curr);
    connect(ui->languageComboBox,
            static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
            this,
            &AppearanceOptionsWidget::onLanguageComboBoxCurrentIndexChanged);

    connect(Config(), &Configuration::fontsUpdated, this,
            &AppearanceOptionsWidget::updateFontFromConfig);
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
    QSignalBlocker signalBlockerColorBox(ui->colorComboBox);
    QSignalBlocker signalBlockerThemeBox(ui->themeComboBox);
     ui->themeComboBox->clear();
    for (auto &it : kCutterQtThemesList) {
        ui->themeComboBox->addItem(it.name);
    }
    int curQtThemeIndex = Config()->getTheme();
    if (curQtThemeIndex >= kCutterQtThemesList.size()) {
        curQtThemeIndex = 0;
        Config()->setTheme(curQtThemeIndex);
    }
    ui->themeComboBox->setCurrentIndex(curQtThemeIndex);

    QList<QString> themes = Core()->getColorThemes();
    ui->colorComboBox->clear();
    for (QString theme : themes) {
        if (ColorSchemeFileWorker().isCustomScheme(theme) ||
            (kCutterQtThemesList[curQtThemeIndex].flag & kRelevantSchemes[theme])) {
            ui->colorComboBox->addItem(theme);
        }
    }

    QString curTheme = Config()->getLastThemeOf(kCutterQtThemesList[curQtThemeIndex]);
    int index = ui->colorComboBox->findText(curTheme);
    if (index == -1) {
        index = 0;
    }

    ui->colorComboBox->setCurrentIndex(index);
    curTheme = ui->colorComboBox->currentText();
    Config()->setColorTheme(curTheme);
    ui->colorSchemePrefWidget->updateSchemeFromConfig();
    int maxThemeLen = 0;
    for (QString str : themes) {
        int strLen = str.length();
        if (strLen > maxThemeLen) {
            maxThemeLen = strLen;
        }
    }
    ui->colorComboBox->setMinimumContentsLength(maxThemeLen);
    ui->colorComboBox->setSizeAdjustPolicy(QComboBox::AdjustToMinimumContentsLength);
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
    Config()->setTheme(index);
    updateThemeFromConfig();
}

void AppearanceOptionsWidget::on_colorComboBox_currentIndexChanged(int index)
{
    QString theme = ui->colorComboBox->itemText(index);

    int curQtThemeIndex = Config()->getTheme();
    if (curQtThemeIndex >= kCutterQtThemesList.size()) {
        curQtThemeIndex = 0;
        Config()->setTheme(curQtThemeIndex);
    }

    Config()->setLastThemeOf(kCutterQtThemesList[curQtThemeIndex], theme);
    Config()->setColorTheme(theme);
    ui->colorSchemePrefWidget->updateSchemeFromConfig();
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
    ui->colorSchemePrefWidget->updateSchemeFromConfig();
    updateThemeFromConfig();
}

void AppearanceOptionsWidget::on_deleteButton_clicked()
{
    if (ColorSchemeFileWorker().isCustomScheme(Config()->getCurrentTheme())) {
        QMessageBox mb;
        mb.setWindowTitle(tr("Delete"));
        mb.setText(tr("Are you sure you want to delete theme ") + Config()->getCurrentTheme());
        mb.setIcon(QMessageBox::Question);
        mb.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
        if (mb.exec() == QMessageBox::Yes) {
            ColorSchemeFileWorker().deleteScheme(Config()->getCurrentTheme());
            updateThemeFromConfig();
        }
    }
}

void AppearanceOptionsWidget::onLanguageComboBoxCurrentIndexChanged(int index)
{
    QString language = ui->languageComboBox->itemText(index).toLower();
    auto allLocales = QLocale::matchingLocales(QLocale::AnyLanguage, QLocale::AnyScript,
                                               QLocale::AnyCountry);

    for (auto &it : allLocales) {
        if (it.nativeLanguageName().toLower() == language) {
            Config()->setLocale(it);
            break;
        }
    }

    QMessageBox mb;
    mb.setWindowTitle(tr("Language settings"));
    mb.setText(tr("Language will be changed after next application start."));
    mb.setIcon(QMessageBox::Information);
    mb.setStandardButtons(QMessageBox::Ok);
    mb.exec();
}
