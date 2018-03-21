
#ifndef GRAPHOPTIONSWIDGET_H
#define GRAPHOPTIONSWIDGET_H

#include <QDialog>
#include <QPushButton>
#include <memory>

#include "Cutter.h"

class PreferencesDialog;

namespace Ui {
class GraphOptionsWidget;
}

class GraphOptionsWidget : public QDialog
{
    Q_OBJECT

public:
    explicit GraphOptionsWidget(PreferencesDialog *dialog, QWidget *parent = nullptr);
    ~GraphOptionsWidget();

private:
    std::unique_ptr<Ui::GraphOptionsWidget> ui;

    void triggerOptionsChanged();

private slots:
    void updateOptionsFromVars();

    void on_maxColsSpinBox_valueChanged(int value);
};


#endif //GRAPHOPTIONSWIDGET_H
