#include "WelcomeDialog.h"

#include "AboutDialog.h"
#include "Configuration.h"
#include "CutterConfig.h"
#include "common/Helpers.h"
#include "ui_WelcomeDialog.h"

#include <QMessageBox>

WelcomeDialog::WelcomeDialog(QWidget *parent) : QDialog(parent), ui(new Ui::WelcomeDialog)
{
    ui->setupUi(this);
    setWindowFlags(windowFlags() & (~Qt::WindowContextHelpButtonHint));
    ui->logoSvgWidget->load(Config()->getLogoFile());
    ui->versionLabel->setText("<font color='#a4a9b2'>" + tr("Version ") + CUTTER_VERSION_FULL
                              + "</font>");
    ui->themeComboBox->setCurrentIndex(Config()->getInterfaceTheme());

    const QSignalBlocker s(ui->updatesCheckBox);
    ui->updatesCheckBox->setChecked(Config()->getAutoUpdateEnabled());

    auto langs = Config()->getAvailableTranslations();
    for (auto &lang : langs) {
        ui->languageComboBox->addItem(lang.name, lang.locale);
    }

    auto matchingLang =
            std::find_if(langs.begin(), langs.end(), [](const Configuration::LangInfo &v) {
                return v.locale == Config()->getCurrLocale();
            });
    if (matchingLang == langs.end()) {
        matchingLang =
                std::find_if(langs.begin(), langs.end(), [](const Configuration::LangInfo &v) {
                    return v.locale.language() == QLocale::English;
                });
    }
    if (matchingLang != langs.end()) {
        ui->languageComboBox->setCurrentIndex(matchingLang - langs.begin());
    } else {
        ui->languageComboBox->setCurrentText("English");
    }

    connect(ui->languageComboBox,
            static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged), this,
            &WelcomeDialog::onLanguageCurrentIndexChanged);

    connect(ui->languageComboBox,
            static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged), this,
            &WelcomeDialog::onLanguageCurrentIndexChanged);

    connect(ui->themeComboBox,
            static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged), this,
            &WelcomeDialog::onThemeComboBoxCurrentIndexChanged);

    connect(ui->checkUpdateButton, &QPushButton::clicked, this,
            &WelcomeDialog::onCheckUpdateButtonClicked);

    connect(ui->continueButton, &QPushButton::clicked, this,
            &WelcomeDialog::onContinueButtonClicked);
#if QT_VERSION >= QT_VERSION_CHECK(6, 9, 0)
    connect(ui->updatesCheckBox, &QCheckBox::checkStateChanged, this,
            &WelcomeDialog::onUpdatesCheckBoxStateChanged);
#else
    connect(ui->updatesCheckBox, &QCheckBox::stateChanged, this,
            &WelcomeDialog::onUpdatesCheckBoxStateChanged);
#endif
    Config()->adjustColorThemeDarkness();
}

WelcomeDialog::~WelcomeDialog() {}

void WelcomeDialog::onThemeComboBoxCurrentIndexChanged(int index)
{
    Config()->setInterfaceTheme(index);

    // make sure that Cutter's logo changes its color according to the selected theme
    ui->logoSvgWidget->load(Config()->getLogoFile());
}

void WelcomeDialog::onLanguageCurrentIndexChanged(int)
{
    const QVariant language = ui->languageComboBox->currentData();
    if (language.canConvert<QLocale>()) {
        Config()->setLocale(language.toLocale());
    }

    QMessageBox mb(this);
    mb.setWindowTitle(tr("Language settings"));
    mb.setText(tr("Language will be changed after next application start."));
    mb.setIcon(QMessageBox::Information);
    mb.setStandardButtons(QMessageBox::Ok);
    mb.exec();
}

void WelcomeDialog::onCheckUpdateButtonClicked()
{
    auto *a = new AboutDialog(this);
    a->setAttribute(Qt::WA_DeleteOnClose);
    a->open();
}

void WelcomeDialog::onContinueButtonClicked()
{
    accept();
}

void WelcomeDialog::onUpdatesCheckBoxStateChanged(int)
{
    Config()->setAutoUpdateEnabled(!Config()->getAutoUpdateEnabled());
}
