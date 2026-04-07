#pragma once

#include <memory>

#include "core/Cutter.h"

class PreferencesDialog;

namespace Ui {
class RizinOptionsWidget;
}

class RizinOptionsWidget : public QDialog
{
    Q_OBJECT

public:
    explicit RizinOptionsWidget(PreferencesDialog *dialog);
    ~RizinOptionsWidget();

private:
    std::unique_ptr<Ui::RizinOptionsWidget> ui;

private slots:
    void updateRizinWidgetsFromConfig();
    void updateCompressProjectConfig();
};
