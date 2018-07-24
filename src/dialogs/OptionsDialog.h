#ifndef OPTIONSDIALOG_H
#define OPTIONSDIALOG_H

#include <QDialog>
#include <QStringList>
#include <QTimer>
#include <QElapsedTimer>
#include <memory>
#include "Cutter.h"
#include "AnalTask.h"
#include "ui_OptionsDialog.h"

class MainWindow;

class OptionsDialog : public QDialog
{
    Q_OBJECT

public:
    explicit OptionsDialog(MainWindow *main);
    ~OptionsDialog();

    QStringList    asm_plugins;

    void setupAndStartAnalysis(int level, QList<QString> advanced);

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
    std::unique_ptr<Ui::OptionsDialog> ui;

    MainWindow *main;
    CutterCore *core;
    int defaultAnalLevel;

    QString analysisDescription(int level);

    void updateCPUComboBox();

public:
    void setInitialScript(const QString &script);

    QString getSelectedArch();
    QString getSelectedCPU();
    int getSelectedBits();
    int getSelectedBBSize();
    InitialOptions::Endianness getSelectedEndianness();
    QString getSelectedOS();
    QList<QString> getSelectedAdvancedAnalCmds();

    void reject() override;
};

#endif // OPTIONSDIALOG_H
