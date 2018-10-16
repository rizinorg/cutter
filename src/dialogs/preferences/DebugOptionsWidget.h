#ifndef DEBUGOPTIONSWIDGET_H
#define DEBUGOPTIONSWIDGET_H
#include <memory>

#include "Cutter.h"
#include "AbstractOptionWidget.h"

class PreferencesDialog;

namespace Ui {
class DebugOptionsWidget;
}

class DebugOptionsWidget : public AbstractOptionWidget
{
    Q_OBJECT

public:
    explicit DebugOptionsWidget(PreferencesDialog *dialog, QWidget *parent = nullptr);
    ~DebugOptionsWidget();

    void apply() override;
    void discard() override;

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

#endif // DEBUGOPTIONSWIDGET_H
