#include "ColorThemeEditDialog.h"
#include "ui_ColorThemeEditDialog.h"

#include "common/ColorThemeWorker.h"
#include "common/Configuration.h"

#include "widgets/ColorThemeListView.h"
#include "widgets/DisassemblyWidget.h"

#include <QScreen>
#include <QKeyEvent>
#include <QSortFilterProxyModel>

ColorThemeEditDialog::ColorThemeEditDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::ColorThemeEditDialog),
    configSignalBlocker(Config()), // Blocks signals from Config to avoid updating of widgets during editing
    colorTheme(Config()->getColorTheme())
{
    showAlphaOptions = {
        "gui.overview.border",
        "gui.overview.fill",
        "wordHighlight",
        "lineHighlight"
    };
    ui->setupUi(this);
    ui->colorComboBox->setShowOnlyCustom(true);

    previewDisasmWidget = new DisassemblyWidget(nullptr);
    previewDisasmWidget->setObjectName("Preview Disasm");
    previewDisasmWidget->setPreviewMode(true);
#if (QT_VERSION >= QT_VERSION_CHECK(5, 12, 0))
    // default size limit is acceptable
    previewDisasmWidget->setMinimumSize(qApp->screenAt(previewDisasmWidget->pos())->size() * 0.5);
#endif
    previewDisasmWidget->setWindowTitle(tr("Disassembly Preview"));
    previewDisasmWidget->setFeatures(QDockWidget::NoDockWidgetFeatures);
    ui->colorPickerAndPreviewLayout->addWidget(previewDisasmWidget);


    connect(ui->colorThemeListView, &ColorThemeListView::blink,
            previewDisasmWidget, &DisassemblyWidget::colorsUpdatedSlot);

    connect(ui->colorThemeListView, &ColorThemeListView::itemChanged,
            this, [this](const QColor& color) {
        ui->colorPicker->updateColor(color);
        QString optionName = ui->colorThemeListView->currentIndex()
                             .data(Qt::UserRole)
                             .value<ColorOption>()
                             .optionName;
        ui->colorPicker->setAlphaEnabled(showAlphaOptions.contains(optionName));
    });

    connect(ui->filterLineEdit, &QLineEdit::textChanged, this,
            [this](const QString& s) {
        static_cast<QSortFilterProxyModel*>(ui->colorThemeListView->model())->setFilterFixedString(s);
    });

    ui->colorThemeListView->setCurrentIndex(ui->colorThemeListView->model()->index(0, 0));

    connect(ui->colorPicker, &ColorPicker::colorChanged, this, &ColorThemeEditDialog::colorOptionChanged);

    connect(ui->colorComboBox, &ColorThemeComboBox::currentTextChanged,
            this, &ColorThemeEditDialog::editThemeChanged);
}

ColorThemeEditDialog::~ColorThemeEditDialog()
{
    delete ui;
    previewDisasmWidget->deleteLater();
}

void ColorThemeEditDialog::accept()
{
    colorTheme = Config()->getColorTheme();
    QJsonDocument sch = ui->colorThemeListView->colorSettingsModel()->getTheme();
    if (ThemeWorker().isCustomTheme(colorTheme)) {
        QString err = ThemeWorker().save(sch, colorTheme);
        if (!err.isEmpty()) {
            QMessageBox::critical(this, tr("Error"), err);
            return;
        }
    }

    configSignalBlocker.unblock();
    Config()->setColorTheme(colorTheme);

    QDialog::accept();
}

void ColorThemeEditDialog::reject()
{
    if (themeWasEdited(ui->colorComboBox->currentText()) &&
        QMessageBox::question(this,
                              tr("Unsaved changes"),
                              tr("Are you sure you want to exit without saving? "
                                 "All changes will be lost.")) == QMessageBox::No) {
        return;
    }
    configSignalBlocker.unblock();
    Config()->setColorTheme(colorTheme);

    QDialog::reject();
}

void ColorThemeEditDialog::keyPressEvent(QKeyEvent *event)
{
    switch (event->key()) {
    case Qt::Key_Escape:
        if (ui->colorPicker->isPickingFromScreen()) {
            ui->colorPicker->stopPickingFromScreen();
        }
        // fallthrough
    case Qt::Key_Return:
        event->accept();
        return;
    default:
        QDialog::keyPressEvent(event);
    }
}

void ColorThemeEditDialog::colorOptionChanged(const QColor& newColor)
{
    QModelIndex currIndex = ui->colorThemeListView->currentIndex();

    if (!currIndex.isValid()) {
        return;
    }

    ColorOption currOption = currIndex.data(Qt::UserRole).value<ColorOption>();
    currOption.color = newColor;
    currOption.changed = true;
    ui->colorThemeListView->model()->setData(currIndex, QVariant::fromValue(currOption));

    Config()->setColor(currOption.optionName, currOption.color);
    if (!ColorThemeWorker::cutterSpecificOptions.contains(currOption.optionName)) {
        Core()->cmd(QString("ec %1 %2").arg(currOption.optionName).arg(currOption.color.name()));
    }
    previewDisasmWidget->colorsUpdatedSlot();
}

void ColorThemeEditDialog::editThemeChanged(const QString& newTheme)
{
    if (themeWasEdited(colorTheme)) {
        int ret = QMessageBox::question(this,
                              tr("Unsaved changes"),
                              tr("Are you sure you want to exit without saving? "
                                 "All changes will be lost."));
        if (ret == QMessageBox::No) {
            QSignalBlocker s(ui->colorComboBox); // avoid second call of this func
            int index = ui->colorComboBox->findText(colorTheme);
            index = index == -1 ? 0 : index;
            ui->colorComboBox->setCurrentIndex(index);
            Config()->setColorTheme(colorTheme);
            return;
        }
    }
    colorTheme = newTheme;
    ui->colorThemeListView->colorSettingsModel()->updateTheme();
    previewDisasmWidget->colorsUpdatedSlot();
    setWindowTitle(tr("Theme Editor - <%1>").arg(colorTheme));
}

bool ColorThemeEditDialog::themeWasEdited(const QString& theme) const
{
    auto model = ui->colorThemeListView->colorSettingsModel();
    return ThemeWorker().getTheme(theme) != model->getTheme();
}
