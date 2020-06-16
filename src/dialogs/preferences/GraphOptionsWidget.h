
#ifndef GRAPHOPTIONSWIDGET_H
#define GRAPHOPTIONSWIDGET_H

#include <QDialog>
#include <QPushButton>
#include <memory>

#include "core/Cutter.h"

class PreferencesDialog;

namespace Ui {
class GraphOptionsWidget;
}

class GraphOptionsWidget : public QDialog
{
    Q_OBJECT

public:
    explicit GraphOptionsWidget(PreferencesDialog *dialog);
    ~GraphOptionsWidget();
private:
    std::unique_ptr<Ui::GraphOptionsWidget> ui;

    void triggerOptionsChanged();

private slots:
    void updateOptionsFromVars();

    void on_maxColsSpinBox_valueChanged(int value);
    void on_graphOffsetCheckBox_toggled(bool checked);

    void checkTransparentStateChanged(int checked);
    void bitmapGraphScaleValueChanged(double value);
    void layoutSpacingChanged();
};


#endif //GRAPHOPTIONSWIDGET_H
