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

    /**
     * @brief This function creates the list with the different options shown in the selector for anal.in
     */
    void createAnalInOptionsList();

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

    /**
     * @brief A slot to update the value of anal.in when a different option is selected
     * @param index - The index of the selected option for anal.in
     */
    void updateAnalIn(int index);

    /**
     * @brief A slot to update the value of anal.ptrdepth when a new value is selected
     * @param value - The new value for anal.ptrdepth
     */
    static void updateAnalPtrDepth(int value);

    /**
     * @brief slot to update the value of anal.prelude when a new value is introduced in the corresponding textbox
     * @param prelude - The new value for anal.prelude
     */
    static void updateAnalPrelude(const QString &prelude);
};

#endif // ANALOPTIONSWIDGET_H
