#ifndef OPTIONSDIALOG_H
#define OPTIONSDIALOG_H

#include <QDialog>
#include <QStringList>

#include "qrcore.h"
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

    void setupAndStartAnalysis(int level);

private slots:
    void on_closeButton_clicked();

    void on_okButton_clicked();

    void on_cancelButton_clicked();

    void anal_finished();

    void on_analSlider_valueChanged(int value);

    void on_AdvOptButton_clicked();

    void on_analCheckBox_clicked(bool checked);

private:
    Ui::OptionsDialog *ui;
    AnalThread analThread;
    MainWindow *main;
    int defaultAnalLevel;

    QString analysisDescription(int level);
};

#endif // OPTIONSDIALOG_H
