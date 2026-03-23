#ifndef SYMBOLSERVERS_H
#define SYMBOLSERVERS_H

#include <QWidget>
#include <QDialog>

class PreferencesDialog;

namespace Ui {
class SymbolServers;
}

class SymbolServers : public QDialog
{
    Q_OBJECT

public:
    explicit SymbolServers(QWidget *parent = nullptr);
    ~SymbolServers();

private slots:
    void on_pdbSelectButton_clicked();

private:
    Ui::SymbolServers *ui;
    void updateDebuginfodLayout();
    void updatePDBLayout();
    void reanalyze();
};

#endif // SYMBOLSERVERS_H
