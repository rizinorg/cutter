#ifndef GRAPHOPTIONSWIDGET_H
#define GRAPHOPTIONSWIDGET_H

#include <QDialog>
#include <QPushButton>

#include <memory>

class PreferencesDialog;

namespace Ui {
class GraphOptionsWidget;
}

/**
 * @brief Contains configurable options related to graphs
 */
class GraphOptionsWidget : public QDialog
{
    Q_OBJECT

public:
    explicit GraphOptionsWidget(PreferencesDialog *dialog);
    ~GraphOptionsWidget();

private:
    std::unique_ptr<Ui::GraphOptionsWidget> ui;

    void triggerOptionsChanged() const;

private slots:
    void updateOptionsFromVars();

    void onMaxColsSpinBoxValueChanged(int value);
    void onMinFontSizeSpinBoxValueChanged(int value);
    void onGraphPreviewCheckBoxToggled(bool checked);

    void checkTransparentStateChanged(int checked);
    void bitmapGraphScaleValueChanged(double value);
    void checkGraphBlockEntryOffsetChanged(bool checked);
    void layoutSpacingChanged();
};

#endif // GRAPHOPTIONSWIDGET_H
