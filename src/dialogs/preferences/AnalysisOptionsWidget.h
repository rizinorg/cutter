#ifndef ANALOPTIONSWIDGET_H
#define ANALOPTIONSWIDGET_H

#include <QDialog>
#include <memory>

#include "core/Cutter.h"

class PreferencesDialog;

namespace Ui {
class AnalysisOptionsWidget;
}

class AnalysisOptionsWidget : public QDialog
{
    Q_OBJECT

public:
    explicit AnalysisOptionsWidget(PreferencesDialog *dialog);
    ~AnalysisOptionsWidget();

private:
    std::unique_ptr<Ui::AnalysisOptionsWidget> ui;
    struct ConfigCheckbox
    {
        QCheckBox *checkBox;
        QString config;
    };
    QList<ConfigCheckbox> checkboxes;

    /**
     * @brief This function creates the list with the different options shown in the selector for
     * analysis.in
     */
    void createAnalysisInOptionsList();

private slots:
    /**
     * @brief A slot to display the options in the dialog according to the current analysis.*
     * configuration
     */
    void updateAnalysisOptionsFromVars();

    /**
     * @brief A generic slot to handle the simple cases where a checkbox is toggled
     * while it's only responsible for a single independent boolean configuration eval.
     * @param checkBox - The checkbox which is responsible for the signal
     * @param config - the configuration string to be toggled
     */
    static void checkboxEnabler(QCheckBox *checkbox, const QString &config);

    /**
     * @brief A slot to update the value of analysis.in when a different option is selected
     * @param index - The index of the selected option for analysis.in
     */
    void updateAnalysisIn(int index);

    /**
     * @brief A slot to update the value of analysis.ptrdepth when a new value is selected
     * @param value - The new value for analysis.ptrdepth
     */
    static void updateAnalysisPtrDepth(int value);

    /**
     * @brief slot to update the value of analysis.prelude when a new value is introduced in the
     * corresponding textbox
     * @param prelude - The new value for analysis.prelude
     */
    static void updateAnalysisPrelude(const QString &prelude);
};

#endif // ANALOPTIONSWIDGET_H
