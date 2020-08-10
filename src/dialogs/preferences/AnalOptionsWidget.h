#ifndef ANALOPTIONSWIDGET_H
#define ANALOPTIONSWIDGET_H

#include <QDialog>
#include <memory>

#include "core/Cutter.h"

class PreferencesDialog;

namespace Ui {
class AnalOptionsWidget;
}

class AnalOptionsWidget : public QDialog
{
    Q_OBJECT

public:
    explicit AnalOptionsWidget(PreferencesDialog *dialog);
    ~AnalOptionsWidget();

private:
    std::unique_ptr<Ui::AnalOptionsWidget> ui;
    struct ConfigCheckbox {
        QCheckBox *checkBox;
        QString config;
    };
    QList<ConfigCheckbox> checkboxes;

private slots:
    /**
     * @brief A slot to display the options in the dialog according to the current anal.* configuration
     */
    void updateAnalOptionsFromVars();

    /**
     * @brief A generic slot to handle the simple cases where a checkbox is toggled
     * while it's only responsible for a single independent boolean configuration eval.
     * @param checkBox - The checkbox which is responsible for the signal
     * @param config - the configuration string to be toggled
     */
    static void checkboxEnabler(QCheckBox *checkbox, const QString &config);
};

#endif // ANALOPTIONSWIDGET_H
