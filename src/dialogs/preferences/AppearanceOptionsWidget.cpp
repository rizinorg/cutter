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

AppearanceOptionsWidget::AppearanceOptionsWidget(PreferencesDialog *dialog, QWidget *parent)
    : QDialog(parent),
      ui(new Ui::AppearanceOptionsWidget)
{
    Q_UNUSED(dialog);
    ui->setupUi(this);

    updateFontFromConfig();
    updateThemeFromConfig(false);

    QStringList langs = Config()->getAvailableTranslations();
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

void AppearanceOptionsWidget::updateThemeFromConfig(bool qtThemeChanged)
{
    // Disconnect currentIndexChanged because clearing the comboxBox and refiling it causes its index to change.
    QSignalBlocker signalBlockerColorBox(ui->colorComboBox);
    QSignalBlocker signalBlockerThemeBox(ui->themeComboBox);
    Q_UNUSED(signalBlockerColorBox);
    Q_UNUSED(signalBlockerThemeBox);

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
    for (const QString &theme : themes) {
        if (ColorSchemeFileWorker().isCustomScheme(theme) ||
            (kCutterQtThemesList[curQtThemeIndex].flag & kRelevantSchemes[theme])) {
            ui->colorComboBox->addItem(theme);
        }
    }

    QString curTheme = qtThemeChanged
        ? Config()->getLastThemeOf(kCutterQtThemesList[curQtThemeIndex])
        : Config()->getColorTheme();
    const int index = ui->colorComboBox->findText(curTheme);

    ui->colorComboBox->setCurrentIndex(index == -1 ? 0 : index);
    if (qtThemeChanged || index == -1) {
        curTheme = ui->colorComboBox->currentText();
        Config()->setColorTheme(curTheme);
    }
    ui->colorSchemePrefWidget->updateSchemeFromConfig();
    int maxThemeLen = 0;
    for (const QString &str : themes) {
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
    } while ((!newSchemeName.isEmpty() && ColorSchemeFileWorker().isNameEngaged(newSchemeName))
        || newSchemeName.contains(QRegExp("[^\\w.()\\[\\]_-]"))
        || newSchemeName.startsWith('.'));

    if (newSchemeName.isEmpty())
        return;
    ColorSchemeFileWorker().copy(Config()->getColorTheme(), newSchemeName);
    Config()->setColorTheme(newSchemeName);
    ui->colorSchemePrefWidget->updateSchemeFromConfig();
    updateThemeFromConfig(false);
}

void AppearanceOptionsWidget::on_deleteButton_clicked()
{
    if (ColorSchemeFileWorker().isCustomScheme(Config()->getColorTheme())) {
        QMessageBox mb;
        mb.setWindowTitle(tr("Delete"));
        mb.setText(tr("Are you sure you want to delete theme ") + Config()->getColorTheme());
        mb.setIcon(QMessageBox::Question);
        mb.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
        if (mb.exec() == QMessageBox::Yes) {
            ColorSchemeFileWorker().deleteScheme(Config()->getColorTheme());
            updateThemeFromConfig(false);
        }
    }
}

void AppearanceOptionsWidget::onLanguageComboBoxCurrentIndexChanged(int index)
{
    QString language = ui->languageComboBox->itemText(index).toLower();
    if (Config()->setLocaleByName(language)) {
        QMessageBox::information(this,
            tr("Language settings"),
            tr("Language will be changed after next application start."));
        return;
    }

    qWarning() << tr("Cannot set language, not found in available ones");
}
