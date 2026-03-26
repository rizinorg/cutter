#ifndef SYMBOLSERVERS_H
#define SYMBOLSERVERS_H

#include <QWidget>
#include <QDialog>

class MainWindow;

class PreferencesDialog;

namespace Ui {
class SymbolServers;
}

class SymbolServers : public QDialog
{
    Q_OBJECT

public:
    explicit SymbolServers(PreferencesDialog *parent = nullptr);
    ~SymbolServers();

private slots:
    void pdbSelectButtonClicked();

private:
    MainWindow *mainWindow;
    Ui::SymbolServers *ui;
    void updateDebuginfodLayout();
    void updatePDBLayout();
    void reanalyze();
};

#endif // SYMBOLSERVERS_H
