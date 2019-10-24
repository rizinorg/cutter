#ifndef OPTIONSDIALOG_H
#define OPTIONSDIALOG_H

#include <QDialog>
#include <QCheckBox>
#include <memory>
#include "common/InitialOptions.h"

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

    void setupAndStartAnalysis(/*int level, QList<QString> advanced*/);

private slots:
    void on_okButton_clicked();
    void on_analSlider_valueChanged(int value);
    void on_AdvOptButton_clicked();
    void on_analCheckBox_clicked(bool checked);
    void on_archComboBox_currentIndexChanged(int index);
    void on_pdbSelectButton_clicked();
    void on_scriptSelectButton_clicked();

    void updatePDBLayout();
    void updateScriptLayout();

protected:
    void closeEvent(QCloseEvent *event) override;

private:
    std::unique_ptr<Ui::InitialOptionsDialog> ui;

    MainWindow *main;
    CutterCore *core;

    QString analysisDescription(int level);
    QString shellcode;
    int analLevel;
    QStringList asm_plugins;


    void updateCPUComboBox();
    struct AnalysisCommands {
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

public:
    void loadOptions(const InitialOptions &options);

    void reject() override;
};

#endif // OPTIONSDIALOG_H
