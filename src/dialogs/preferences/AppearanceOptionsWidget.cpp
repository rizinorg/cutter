#include "AppearanceOptionsWidget.h"

#include "PreferencesDialog.h"
#include "common/ColorThemeWorker.h"
#include "common/Configuration.h"
#include "common/Helpers.h"
#include "dialogs/preferences/ColorThemeEditDialog.h"
#include "ui_AppearanceOptionsWidget.h"

#include <QComboBox>
#include <QDir>
#include <QFile>
#include <QFileDialog>
#include <QFontDialog>
#include <QInputDialog>
#include <QLabel>
#include <QPainter>
#include <QSignalBlocker>
#include <QStandardPaths>
#include <QTranslator>
#include <QtSvg/QSvgRenderer>
#include <QtWidgets/QSpinBox>

AppearanceOptionsWidget::AppearanceOptionsWidget(PreferencesDialog *dialog)
    : QDialog(dialog), ui(new Ui::AppearanceOptionsWidget)
{
    ui->setupUi(this);
    updateFromConfig();

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

    auto setIcons = [this]() {
        const QColor textColor = palette().text().color();
        ui->editButton->setIcon(getIconFromSvg(":/img/icons/pencil_thin.svg", textColor));
        ui->deleteButton->setIcon(getIconFromSvg(":/img/icons/trash_bin.svg", textColor));
        ui->copyButton->setIcon(getIconFromSvg(":/img/icons/copy.svg", textColor));
        ui->importButton->setIcon(getIconFromSvg(":/img/icons/download_black.svg", textColor));
        ui->exportButton->setIcon(getIconFromSvg(":/img/icons/upload_black.svg", textColor));
        ui->renameButton->setIcon(getIconFromSvg(":/img/icons/rename.svg", textColor));
    };
    setIcons();
    connect(Config(), &Configuration::interfaceThemeChanged, this, setIcons);

    connect(ui->languageComboBox,
            static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged), this,
            &AppearanceOptionsWidget::onLanguageComboBoxCurrentIndexChanged);

    connect(Config(), &Configuration::fontsUpdated, this,
            &AppearanceOptionsWidget::updateFontFromConfig);

    connect(ui->colorComboBox, &QComboBox::currentTextChanged, this,
            &AppearanceOptionsWidget::updateModificationButtons);

    connect(ui->fontZoomBox, static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged), this,
            &AppearanceOptionsWidget::onFontZoomBoxValueChanged);

    ui->useDecompilerHighlighter->setChecked(Config()->isDecompilerAnnotationHighlighterEnabled());
    connect(ui->useDecompilerHighlighter, &QCheckBox::toggled, this,
            [](bool checked) { Config()->enableDecompilerAnnotationHighlighter(checked); });

    connect(ui->themeComboBox,
            static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged), this,
            &AppearanceOptionsWidget::onThemeComboBoxCurrentIndexChanged);

    connect(ui->fontSelectionButton, &QPushButton::clicked, this,
            &AppearanceOptionsWidget::onFontSelectionButtonClicked);

    connect(ui->editButton, &QPushButton::clicked, this,
            &AppearanceOptionsWidget::onEditButtonClicked);
    connect(ui->copyButton, &QPushButton::clicked, this,
            &AppearanceOptionsWidget::onCopyButtonClicked);
    connect(ui->deleteButton, &QPushButton::clicked, this,
            &AppearanceOptionsWidget::onDeleteButtonClicked);
    connect(ui->renameButton, &QPushButton::clicked, this,
            &AppearanceOptionsWidget::onRenameButtonClicked);
    connect(ui->importButton, &QPushButton::clicked, this,
            &AppearanceOptionsWidget::onImportButtonClicked);
    connect(ui->exportButton, &QPushButton::clicked, this,
            &AppearanceOptionsWidget::onExportButtonClicked);
}

AppearanceOptionsWidget::~AppearanceOptionsWidget() {}

void AppearanceOptionsWidget::updateFontFromConfig()
{
    const QFont currentFont = Config()->getBaseFont();
    ui->fontSelectionLabel->setText(currentFont.toString());
}

void AppearanceOptionsWidget::updateThemeFromConfig(bool interfaceThemeChanged)
{
    // Disconnect currentIndexChanged because clearing the comboxBox and refiling it causes its
    // index to change.
    const QSignalBlocker signalBlockerThemeBox(ui->themeComboBox);

    ui->themeComboBox->clear();
    for (auto &it : Configuration::cutterInterfaceThemesList()) {
        ui->themeComboBox->addItem(it.name);
    }
    int currInterfaceThemeIndex = Config()->getInterfaceTheme();
    if (currInterfaceThemeIndex >= Configuration::cutterInterfaceThemesList().size()) {
        currInterfaceThemeIndex = 0;
        Config()->setInterfaceTheme(currInterfaceThemeIndex);
    }
    ui->themeComboBox->setCurrentIndex(currInterfaceThemeIndex);
    ui->colorComboBox->updateFromConfig(interfaceThemeChanged);
    updateModificationButtons(ui->colorComboBox->currentText());
}

void AppearanceOptionsWidget::onFontZoomBoxValueChanged(int zoom)
{
    const qreal zoomFactor = zoom / 100.0;
    Config()->setZoomFactor(zoomFactor);
}

void AppearanceOptionsWidget::onFontSelectionButtonClicked()
{
    const QFont currentFont = Config()->getBaseFont();
    bool ok;
    const QFont newFont = QFontDialog::getFont(&ok, currentFont, this, QString(),
                                               QFontDialog::DontUseNativeDialog);
    if (ok) {
        Config()->setFont(newFont);
    }
}

void AppearanceOptionsWidget::onThemeComboBoxCurrentIndexChanged(int index)
{
    Config()->setInterfaceTheme(index);
    updateThemeFromConfig();
}

void AppearanceOptionsWidget::onEditButtonClicked()
{
    ColorThemeEditDialog dial;
    dial.setWindowTitle(tr("Theme Editor - <%1>").arg(ui->colorComboBox->currentText()));
    dial.exec();
    ui->colorComboBox->updateFromConfig(false);
}

void AppearanceOptionsWidget::onCopyButtonClicked()
{
    const QString currColorTheme = ui->colorComboBox->currentText();

    QString newThemeName;
    do {
        newThemeName = QInputDialog::getText(this, tr("Enter theme name"), tr("Name:"),
                                             QLineEdit::Normal, currColorTheme + tr(" - copy"))
                               .trimmed();
        if (newThemeName.isEmpty()) {
            return;
        }
        if (ThemeWorker().isThemeExist(newThemeName)) {
            QMessageBox::information(this, tr("Theme Copy"),
                                     tr("Theme named %1 already exists.").arg(newThemeName));
        } else {
            break;
        }
    } while (true);

    ThemeWorker().copy(currColorTheme, newThemeName);
    Config()->setColorTheme(newThemeName);
    updateThemeFromConfig(false);
}

void AppearanceOptionsWidget::onDeleteButtonClicked()
{
    const QString currTheme = ui->colorComboBox->currentText();
    if (!ThemeWorker().isCustomTheme(currTheme)) {
        QMessageBox::critical(this, tr("Error"), ThemeWorker().deleteTheme(currTheme));
        return;
    }
    const int ret = QMessageBox::question(
            this, tr("Delete"), tr("Are you sure you want to delete <b>%1</b>?").arg(currTheme));
    if (ret == QMessageBox::Yes) {
        const QString err = ThemeWorker().deleteTheme(currTheme);
        updateThemeFromConfig(false);
        if (!err.isEmpty()) {
            QMessageBox::critical(this, tr("Error"), err);
        }
    }
}

void AppearanceOptionsWidget::onImportButtonClicked()
{
    const QString fileName = QFileDialog::getOpenFileName(
            this, "", QStandardPaths::writableLocation(QStandardPaths::HomeLocation));
    if (fileName.isEmpty()) {
        return;
    }

    const QString err = ThemeWorker().importTheme(fileName);
    const QString themeName = QFileInfo(fileName).fileName();
    if (err.isEmpty()) {
        QMessageBox::information(
                this, tr("Success"),
                tr("Color theme <b>%1</b> was successfully imported.").arg(themeName));
        Config()->setColorTheme(themeName);
        updateThemeFromConfig(false);
    } else {
        QMessageBox::critical(this, tr("Error"), err);
    }
}

void AppearanceOptionsWidget::onExportButtonClicked()
{
    const QString theme = ui->colorComboBox->currentText();
    const QString file = QFileDialog::getSaveFileName(
            this, "",
            QStandardPaths::writableLocation(QStandardPaths::HomeLocation) + QDir::separator()
                    + theme);
    if (file.isEmpty()) {
        return;
    }

    // User already gave his consent for this in QFileDialog::getSaveFileName()
    if (QFileInfo::exists(file)) {
        QFile(file).remove();
    }
    const QString err = ThemeWorker().save(ThemeWorker().getTheme(theme), file);
    if (err.isEmpty()) {
        QMessageBox::information(this, tr("Success"),
                                 tr("Color theme <b>%1</b> was successfully exported.").arg(theme));
    } else {
        QMessageBox::critical(this, tr("Error"), err);
    }
}

void AppearanceOptionsWidget::onRenameButtonClicked()
{
    const QString currColorTheme = Config()->getColorTheme();
    const QString newName = QInputDialog::getText(this, tr("Enter new theme name"), tr("Name:"),
                                                  QLineEdit::Normal, currColorTheme);
    if (newName.isEmpty() || newName == currColorTheme) {
        return;
    }

    const QString err = ThemeWorker().renameTheme(currColorTheme, newName);
    if (!err.isEmpty()) {
        QMessageBox::critical(this, tr("Error"), err);
    } else {
        Config()->setColorTheme(newName);
        updateThemeFromConfig(false);
    }
}

void AppearanceOptionsWidget::onLanguageComboBoxCurrentIndexChanged(int)
{
    const QVariant language = ui->languageComboBox->currentData();
    if (language.canConvert<QLocale>()) {
        Config()->setLocale(language.toLocale());
        QMessageBox::information(this, tr("Language settings"),
                                 tr("Language will be changed after next application start."));
    } else {
        qWarning() << tr("Cannot set language, not found in available ones");
    }
}

void AppearanceOptionsWidget::updateModificationButtons(const QString &theme)
{
    const bool editable = ThemeWorker().isCustomTheme(theme);
    ui->editButton->setEnabled(editable);
    ui->deleteButton->setEnabled(editable);
    ui->renameButton->setEnabled(editable);
}

void AppearanceOptionsWidget::updateFromConfig()
{
    updateFontFromConfig();
    updateThemeFromConfig(false);
    ui->fontZoomBox->setValue(qRound(Config()->getZoomFactor() * 100));
}

QIcon AppearanceOptionsWidget::getIconFromSvg(const QString &fileName, const QColor &after,
                                              const QColor &before)
{
    QFile file(fileName);
    if (!file.open(QIODevice::ReadOnly)) {
        return QIcon();
    }
    QString data = file.readAll();
    data.replace(QRegularExpression(QString("#%1").arg(before.isValid() ? before.name().remove(0, 1)
                                                                        : "[0-9a-fA-F]{6}")),
                 QString("%1").arg(after.name()));

    QSvgRenderer svgRenderer(data.toUtf8());
    QPixmap pix(svgRenderer.defaultSize());
    pix.fill(Qt::transparent);

    QPainter pixPainter(&pix);
    svgRenderer.render(&pixPainter);

    return pix;
}
