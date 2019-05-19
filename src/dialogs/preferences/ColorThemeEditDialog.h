#ifndef COLORTHEMEEDITDIALOG_H
#define COLORTHEMEEDITDIALOG_H

#include <QDialog>

class DisassemblyWidget;

namespace Ui {
class ColorThemeEditDialog;
}

class ColorThemeEditDialog : public QDialog
{
    Q_OBJECT

public:
    explicit ColorThemeEditDialog(QWidget *parent = nullptr);
    ~ColorThemeEditDialog() override;

public slots:
    void accept() override;
    void reject() override;

protected slots:
    void keyPressEvent(QKeyEvent *event) override;

private slots:
    /**
     * @brief Sets @a newColor color for current option.
     * @param newColor
     * New color for current color option.
     */
    void colorOptionChanged(const QColor &newColor);

    /**
     * @brief Changes current theme to edit.
     * @param newTheme
     * Name of new theme.
     */
    void editThemeChanged(const QString &newTheme);

private:
    bool themeWasEdited(const QString &theme) const;

private:
    Ui::ColorThemeEditDialog *ui;
    QSignalBlocker configSignalBlocker;
    DisassemblyWidget *previewDisasmWidget;
    QString colorTheme;
};

#endif // COLORTHEMEEDITDIALOG_H
