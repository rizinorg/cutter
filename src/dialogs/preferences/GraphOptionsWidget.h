
#ifndef GRAPHOPTIONSWIDGET_H
#define GRAPHOPTIONSWIDGET_H

#include <QDialog>
#include <QPushButton>
#include <memory>

#include "Cutter.h"
#include "AbstractOptionWidget.h"

class PreferencesDialog;

namespace Ui {
class GraphOptionsWidget;
}

class GraphOptionsWidget : public AbstractOptionWidget
{
    Q_OBJECT

public:
    explicit GraphOptionsWidget(PreferencesDialog *dialog, QWidget *parent = nullptr);
    ~GraphOptionsWidget();

    void apply() override;
    void discard() override;

private:
    std::unique_ptr<Ui::GraphOptionsWidget> ui;

    void triggerOptionsChanged();

private slots:
    void updateOptionsFromVars();

    void on_maxColsSpinBox_valueChanged(int value);
    void on_graphOffsetCheckBox_toggled(bool checked);
};


#endif //GRAPHOPTIONSWIDGET_H
