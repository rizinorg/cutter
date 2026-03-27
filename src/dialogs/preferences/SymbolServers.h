#ifndef SYMBOLSERVERS_H
#define SYMBOLSERVERS_H

#include <memory>

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
    std::unique_ptr<Ui::SymbolServers> ui;
    void updateDebuginfodLayout();
    void updatePDBLayout();
    void reanalyze();
};

#endif // SYMBOLSERVERS_H
