#ifndef OPTIONSDIALOG_H
#define OPTIONSDIALOG_H

#include <QDialog>
#include <QStringList>
#include <QTimer>
#include <QElapsedTimer>
#include <memory>
#include "Cutter.h"
#include "AnalTask.h"
#include "common/InitialOptions.h"

namespace Ui {
class InitialOptionsDialog;
}

class MainWindow;

class InitialOptionsDialog : public QDialog
{
    Q_OBJECT

public:
    explicit InitialOptionsDialog(MainWindow *main);
    ~InitialOptionsDialog();

    QStringList    asm_plugins;

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

    void updateCPUComboBox();

    QString getSelectedArch();
    QString getSelectedCPU();
    int getSelectedBits();
    int getSelectedBBSize();
    InitialOptions::Endianness getSelectedEndianness();
    QString getSelectedOS();
    QList<QString> getSelectedAdvancedAnalCmds();

public:
    void loadOptions(const InitialOptions &options);

    void reject() override;
};

#endif // OPTIONSDIALOG_H
