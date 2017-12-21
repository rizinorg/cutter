#ifndef OPTIONSDIALOG_H
#define OPTIONSDIALOG_H

#include <QDialog>
#include <QStringList>
#include <QTimer>
#include <QElapsedTimer>
#include <memory>
#include "cutter.h"
#include "AnalThread.h"
#include "ui_OptionsDialog.h"

class MainWindow;

class OptionsDialog : public QDialog
{
    Q_OBJECT

public:
    explicit OptionsDialog(MainWindow *main);
    ~OptionsDialog();
    RAnalFunction functionAt(ut64 addr);
    QStringList    asm_plugins;

    void setupAndStartAnalysis(int level, QList<QString> advanced);

public slots:
    void updateProgress(const QString &str);
private slots:
    void on_okButton_clicked();
    void on_analSlider_valueChanged(int value);
    void on_AdvOptButton_clicked();
    void on_analCheckBox_clicked(bool checked);
    void on_archComboBox_currentIndexChanged(int index);
    void on_pdbSelectButton_clicked();

    void updateProgressTimer();

    void updatePDBLayout();

    void anal_finished();

private:
    AnalThread analThread;
    MainWindow *main;
    CutterCore *core;
    int defaultAnalLevel;

    QString analysisDescription(int level);

    void updateCPUComboBox();

    void setInteractionEnabled(bool enabled);

public:
    std::unique_ptr<Ui::OptionsDialog> ui;
    QString getSelectedArch();
    QString getSelectedCPU();
    int getSelectedBits();
    QString getSelectedOS();
    QTimer analTimer;
    QElapsedTimer analElapsedTimer;
    void reject() override;
};

#endif // OPTIONSDIALOG_H
