#ifndef OPTIONSDIALOG_H
#define OPTIONSDIALOG_H

#include <QDialog>
#include <QStringList>

#include "qrcore.h"
#include "analthread.h"

class MainWindow;

namespace Ui {
class OptionsDialog;
}

class OptionsDialog : public QDialog
{
    Q_OBJECT

public:
    QRCore *core;
    explicit OptionsDialog(QString filename, QWidget *parent = 0);
    ~OptionsDialog();
    RAnalFunction functionAt(ut64 addr);
    QStringList    asm_plugins;

private slots:
    void on_closeButton_clicked();

    void on_okButton_clicked();

    void on_cancelButton_clicked();

    void anal_finished();

    void on_analSlider_valueChanged(int value);

    void on_AdvOptButton_clicked();

private:
    int anal_level;
    QString filename;
    QString shortfn;
    Ui::OptionsDialog *ui;
    AnalThread analThread;
    MainWindow *w;

    void setFilename(QString fn, QString shortfn);
    void setFilename(QString fn);
};

#endif // OPTIONSDIALOG_H
