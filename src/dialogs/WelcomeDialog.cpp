#include "MainWindow.h"
#include "CutterConfig.h"

#include "common/Helpers.h"
#include "WelcomeDialog.h"
#include "ui_WelcomeDialog.h"

WelcomeDialog::WelcomeDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::WelcomeDialog)
{
    ui->setupUi(this);
    ui->logoSvgWidget->load(Config()->getLogoFile());
    ui->versionLabel->setText("<font color='#a4a9b2'>" + tr("Version ") + CUTTER_VERSION_FULL + "</font>");
    ui->themeComboBox->setCurrentIndex(Config()->getTheme());
    ui->themeComboBox->setFixedWidth(200);
    ui->themeComboBox->view()->setFixedWidth(200);

    QStringList langs = Core()->getAvailableTranslations();
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
            &WelcomeDialog::onLanguageComboBox_currentIndexChanged);

}

WelcomeDialog::~WelcomeDialog()
{
    delete ui;
}

void WelcomeDialog::on_themeComboBox_currentIndexChanged(int index)
{
    Config()->setTheme(index);
    ui->logoSvgWidget->load(Config()->getLogoFile());
}


void WelcomeDialog::onLanguageComboBox_currentIndexChanged(int index)
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
