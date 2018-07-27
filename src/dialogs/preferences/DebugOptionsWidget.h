
#pragma once

#include <memory>

#include "Cutter.h"

class PreferencesDialog;

namespace Ui {
class DebugOptionsWidget;
}

class DebugOptionsWidget : public QDialog
{
    Q_OBJECT

public:
    explicit DebugOptionsWidget(PreferencesDialog *dialog, QWidget *parent = nullptr);
    ~DebugOptionsWidget();

private:
    std::unique_ptr<Ui::DebugOptionsWidget> ui;

private slots:
    void updateDebugPlugin();
    void updateDebugArgs();
    void updateStackAddr();
    void updateStackSize();
    void on_pluginComboBox_currentIndexChanged(const QString &index);
    void on_esilBreakOnInvalid_toggled(bool checked);
};
