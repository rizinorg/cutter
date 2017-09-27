#ifndef OPTIONSDIALOG_H
#define OPTIONSDIALOG_H

#include <QDialog>
#include <QStringList>
#include "cutter.h"
#include "analthread.h"

class MainWindow;

namespace Ui
{
    class OptionsDialog;
}

class OptionsDialog : public QDialog
{
    Q_OBJECT

public:
    explicit OptionsDialog(MainWindow *main);
    ~OptionsDialog();
    RAnalFunction functionAt(ut64 addr);
    QStringList    asm_plugins;

    void setupAndStartAnalysis(int level, QList<QString> advanced);

private slots:
    void on_closeButton_clicked();
    void on_okButton_clicked();
    void on_cancelButton_clicked();
    void on_analSlider_valueChanged(int value);
    void on_AdvOptButton_clicked();
    void on_analCheckBox_clicked(bool checked);
    void on_archComboBox_currentIndexChanged(int index);
    void on_pdbSelectButton_clicked();

    void updatePDBLayout();

    void anal_finished();

private:
    Ui::OptionsDialog *ui;
    AnalThread analThread;
    MainWindow *main;
    int defaultAnalLevel;

    QString analysisDescription(int level);

    void updateCPUComboBox();
    QString getSelectedArch();
    QString getSelectedCPU();
    int getSelectedBits();
    QString getSelectedOS();
};

#endif // OPTIONSDIALOG_H
