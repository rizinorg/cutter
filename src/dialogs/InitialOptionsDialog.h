#ifndef OPTIONSDIALOG_H
#define OPTIONSDIALOG_H

#include "common/InitialOptions.h"

#include <QCheckBox>
#include <QDialog>

#include <memory>

namespace Ui {
class InitialOptionsDialog;
}

class CutterCore;
class MainWindow;
class InitialOptionsDialog : public QDialog
{
    Q_OBJECT

public:
    explicit InitialOptionsDialog(MainWindow *main);
    ~InitialOptionsDialog();

    void setupAndStartAnalysis();

private slots:
    void onOkButtonClicked();
    void onAnalysisSliderValueChanged(int value);
    void onAdvOptButtonClicked();
    void onAnalysisCheckBoxClicked(bool checked);
    void onArchComboBoxCurrentIndexChanged(int index);
    void onPdbSelectButtonClicked();
    void onScriptSelectButtonClicked();

    void updatePDBLayout();
    void updateScriptLayout();
    void updateDebuginfodLayout();

protected:
    void closeEvent(QCloseEvent *event) override;

private:
    std::unique_ptr<Ui::InitialOptionsDialog> ui;

    MainWindow *main;

    QString analysisDescription(int level);
    QString shellcode;
    int analysisLevel;
    QList<RzAsmPluginDescription> asmPlugins;

    void updateCPUComboBox();
    struct AnalysisCommands
    {
        CommandDescription commandDesc;
        QCheckBox *checkbox;
        bool checked;
    };
    QList<AnalysisCommands> analysisCommands;

    QList<QString> getAnalysisCommands(const InitialOptions &options);
    QString getSelectedArch() const;
    QString getSelectedCPU() const;
    int getSelectedBits() const;
    InitialOptions::Endianness getSelectedEndianness() const;
    QString getSelectedOS() const;
    QList<CommandDescription> getSelectedAdvancedAnalCmds() const;

    /**
     * @brief setTooltipWithConfigHelp is an helper function that add a tolltip to a widget with
     * a description of a given Rizin eval config.
     * @param w - a widget to which to add the tooltip
     * @param config - name of a configuration variable such as "asm.bits".
     */
    void setTooltipWithConfigHelp(QWidget *w, const char *config);

public:
    void loadOptions(const InitialOptions &options);

    void reject() override;
};

#endif // OPTIONSDIALOG_H
