#include <QDir>
#include <QFile>
#include <QLabel>
#include <QPainter>
#include <QFontDialog>
#include <QFileDialog>
#include <QTranslator>
#include <QInputDialog>
#include <QSignalBlocker>
#include <QStandardPaths>
#include <QtSvg/QSvgRenderer>

#include <QComboBox>
#include "PreferencesDialog.h"
#include "AppearanceOptionsWidget.h"
#include "ui_AppearanceOptionsWidget.h"

#include "common/Helpers.h"
#include "common/Configuration.h"

#include "common/ColorThemeWorker.h"
#include "dialogs/preferences/ColorThemeEditDialog.h"
#include "widgets/ColorPicker.h"

AppearanceOptionsWidget::AppearanceOptionsWidget(PreferencesDialog *dialog)
    : QDialog(dialog),
      ui(new Ui::AppearanceOptionsWidget)
{
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

    auto setIcons = [this]() {
        QColor textColor = palette().text().color();
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
            static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
            this,
            &AppearanceOptionsWidget::onLanguageComboBoxCurrentIndexChanged);

    connect(Config(), &Configuration::fontsUpdated, this,
            &AppearanceOptionsWidget::updateFontFromConfig);

    connect(ui->colorComboBox, &QComboBox::currentTextChanged,
            this, &AppearanceOptionsWidget::updateModificationButtons);
}

AppearanceOptionsWidget::~AppearanceOptionsWidget() {}

void AppearanceOptionsWidget::updateFontFromConfig()
{
    QFont currentFont = Config()->getFont();
    ui->fontSelectionLabel->setText(currentFont.toString());
}

void AppearanceOptionsWidget::updateThemeFromConfig(bool interfaceThemeChanged)
{
    // Disconnect currentIndexChanged because clearing the comboxBox and refiling it causes its index to change.
    QSignalBlocker signalBlockerThemeBox(ui->themeComboBox);

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
    Config()->setInterfaceTheme(index);
    updateThemeFromConfig();
}

void AppearanceOptionsWidget::on_editButton_clicked()
{
    ColorThemeEditDialog dial;
    dial.setWindowTitle(tr("Theme Editor - <%1>").arg(ui->colorComboBox->currentText()));
    dial.exec();
    ui->colorComboBox->updateFromConfig(false);
}

void AppearanceOptionsWidget::on_copyButton_clicked()
{
    QString currColorTheme = ui->colorComboBox->currentText();

    QString newThemeName;
    do {
        newThemeName = QInputDialog::getText(this, tr("Enter theme name"),
                                             tr("Name:"), QLineEdit::Normal,
                                             currColorTheme + tr(" - copy"))
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

void AppearanceOptionsWidget::on_deleteButton_clicked()
{
    QString currTheme = ui->colorComboBox->currentText();
    if (!ThemeWorker().isCustomTheme(currTheme)) {
        QMessageBox::critical(nullptr, tr("Error"), ThemeWorker().deleteTheme(currTheme));
        return;
    }
    int ret = QMessageBox::question(nullptr,
                                    tr("Delete"),
                                    tr("Are you sure you want to delete <b>%1</b>?")
                                    .arg(currTheme));
    if (ret == QMessageBox::Yes) {
        QString err = ThemeWorker().deleteTheme(currTheme);
        updateThemeFromConfig(false);
        if (!err.isEmpty()) {
            QMessageBox::critical(nullptr, tr("Error"), err);
        }
    }
}

void AppearanceOptionsWidget::on_importButton_clicked()
{
    QString fileName = QFileDialog::getOpenFileName(this,
                                                    "",
                                                    QStandardPaths::writableLocation(QStandardPaths::HomeLocation));
    if (fileName.isEmpty()) {
        return;
    }

    QString err = ThemeWorker().importTheme(fileName);
    QString themeName = QFileInfo(fileName).fileName();
    if (err.isEmpty()) {
        QMessageBox::information(this,
                                 tr("Success"),
                                 tr("Color theme <b>%1</b> was successfully imported.").arg(themeName));
        Config()->setColorTheme(themeName);
        updateThemeFromConfig(false);
    } else {
        QMessageBox::critical(this, tr("Error"), err);
    }
}

void AppearanceOptionsWidget::on_exportButton_clicked()
{
    QString theme = ui->colorComboBox->currentText();
    QString file = QFileDialog::getSaveFileName(this,
                                                "",
                                                QStandardPaths::writableLocation(QStandardPaths::HomeLocation)
                                                + QDir::separator() + theme);
    if (file.isEmpty()) {
        return;
    }

    // User already gave his consent for this in QFileDialog::getSaveFileName()
    if (QFileInfo(file).exists()) {
        QFile(file).remove();
    }
    QString err = ThemeWorker().save(ThemeWorker().getTheme(theme), file);
    if (err.isEmpty()) {
        QMessageBox::information(this,
                                 tr("Success"),
                                 tr("Color theme <b>%1</b> was successfully exported.").arg(theme));
    } else {
        QMessageBox::critical(this, tr("Error"), err);
    }
}

void AppearanceOptionsWidget::on_renameButton_clicked()
{
    QString currColorTheme = Config()->getColorTheme();
    QString newName = QInputDialog::getText(this,
                                            tr("Enter new theme name"),
                                            tr("Name:"),
                                            QLineEdit::Normal,
                                            currColorTheme);
    if (newName.isEmpty() || newName == currColorTheme) {
        return;
    }

    QString err = ThemeWorker().renameTheme(currColorTheme, newName);
    if (!err.isEmpty()) {
        QMessageBox::critical(this, tr("Error"), err);
    } else {
        Config()->setColorTheme(newName);
        updateThemeFromConfig(false);
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

void AppearanceOptionsWidget::updateModificationButtons(const QString& theme)
{
    bool editable = ThemeWorker().isCustomTheme(theme);
    ui->editButton->setEnabled(editable);
    ui->deleteButton->setEnabled(editable);
    ui->renameButton->setEnabled(editable);
}

QIcon AppearanceOptionsWidget::getIconFromSvg(const QString& fileName, const QColor& after, const QColor& before)
{
    QFile file(fileName);
    if (!file.open(QIODevice::ReadOnly)) {
        return QIcon();
    }
    QString data = file.readAll();
    data.replace(QRegExp(QString("#%1").arg(before.isValid() ? before.name().remove(0, 1) : "[0-9a-fA-F]{6}")),
                 QString("%1").arg(after.name()));

    QSvgRenderer svgRenderer(data.toUtf8());
    QPixmap pix(svgRenderer.defaultSize());
    pix.fill(Qt::transparent);

    QPainter pixPainter(&pix);
    svgRenderer.render(&pixPainter);

    return pix;
}
