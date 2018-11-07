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

static inline size_t qtThemeToFlag(const CutterQtThemes& t)
{
    return t + 1;
}

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


//static constexpr int lightTheme = 4; // If we finally add light theme
static constexpr int defaultTheme = 1;
static constexpr int darkTheme = 2;


bool shouldShow(const QString &s)
{
    static const QMap<QString, size_t> relevantSchemes = {
        { "zenburn", qtThemeToFlag(darkTheme) },
        { "darkda", qtThemeToFlag(darkTheme) },
        { "solarized", qtThemeToFlag(darkTheme) },
        { "onedark", qtThemeToFlag(darkTheme) },
        { "consonance", qtThemeToFlag(darkTheme) },
        { "ayu", qtThemeToFlag(darkTheme) },

        { "cutter", qtThemeToFlag(defaultTheme) },
        { "tango", qtThemeToFlag(defaultTheme) },
        { "white", qtThemeToFlag(defaultTheme) },
        { "dark", qtThemeToFlag(defaultTheme) },
        { "matrix", qtThemeToFlag(defaultTheme) }
    };

    size_t res = relevantSchemes[s];
    if ((Config()->getTheme() == darkTheme &&
            res & qtThemeToFlag(darkTheme)) ||
            (Config()->getTheme() == defaultTheme &&
             res & qtThemeToFlag(defaultTheme))) {
        return true;
    }

    return false;
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

    ui->colorSchemePrefWidget->setNewScheme(Config()->getCurrentTheme());
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
    disconnect(ui->colorComboBox, SIGNAL(currentIndexChanged(int)),
               this, SLOT(on_colorComboBox_currentIndexChanged(int)));
    disconnect(ui->colorComboBox, &QComboBox::currentTextChanged,
               ui->colorSchemePrefWidget, &ColorSchemePrefWidget::setNewScheme);
    disconnect(ui->themeComboBox, SIGNAL(currentIndexChanged(int)),
               this, SLOT(on_themeComboBox_currentIndexChanged(int)));

    CutterQtThemes curQtTheme = CutterQtThemes(Config()->getTheme());
    if (curQtTheme == defaultTheme)
        ui->themeComboBox->setCurrentText("Default");
    else if (curQtTheme == darkTheme)
        ui->themeComboBox->setCurrentText("Dark");
    else if (curQtTheme == lightTheme)
        ui->themeComboBox->setCurrentText("Light");

    QList<QString> themes = Core()->getColorThemes();
    ui->colorComboBox->clear();
    //    ui->colorComboBox->addItem("default");
    for (QString str : themes)
        if (ColorSchemeFileWorker().isCustomScheme(str) || shouldShow(str))
            ui->colorComboBox->addItem(str);
        }
    }
    QString curTheme = Config()->getCurrentTheme();
    int index = ui->colorComboBox->findText(curTheme);
    if (index == -1)
        index = 0;

    connect(ui->colorComboBox, SIGNAL(currentIndexChanged(int)),
            this, SLOT(on_colorComboBox_currentIndexChanged(int)));
    connect(ui->colorComboBox, &QComboBox::currentTextChanged,
            ui->colorSchemePrefWidget, &ColorSchemePrefWidget::setNewScheme);
    connect(ui->themeComboBox, SIGNAL(currentIndexChanged(int)),
            this, SLOT(on_themeComboBox_currentIndexChanged(int)));

    ui->colorComboBox->setCurrentIndex(index);
    curTheme = ui->colorComboBox->currentText();
    Config()->setColorTheme(curTheme);
    ui->colorSchemePrefWidget->setNewScheme(curTheme);
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
    QString theme = ui->themeComboBox->itemText(index);
    int th = defaultTheme;

    if (theme == "Dark") {
        th = darkTheme;
    } else if (theme == "Light") {
        th = lightTheme;
    }

    Config()->setTheme(th);
}

void AppearanceOptionsWidget::on_colorComboBox_currentIndexChanged(int index)
{
    QString theme = ui->colorComboBox->itemText(index);
    Config()->setLastThemeOf(CutterQtThemes(Config()->getTheme()), theme);
    ui->colorSchemePrefWidget->setNewScheme(theme);
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
    ui->colorSchemePrefWidget->setNewScheme(newSchemeName);
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
