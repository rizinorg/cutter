#ifndef SYMBOLSOPTIONSWIDGET_H
#define SYMBOLSOPTIONSWIDGET_H

#include <memory>

#include <QWidget>
#include <QDialog>

class MainWindow;

class PreferencesDialog;

namespace Ui {
class SymbolsOptionsWidget;
}

class SymbolsOptionsWidget : public QDialog
{
    Q_OBJECT

public:
    explicit SymbolsOptionsWidget(PreferencesDialog *parent = nullptr);
    ~SymbolsOptionsWidget();

private slots:
    void pdbSelectButtonClicked();

private:
    MainWindow *mainWindow;
    std::unique_ptr<Ui::SymbolsOptionsWidget> ui;
    void updateDebuginfodLayout();
    void reanalyze();
};

#endif // SYMBOLSOPTIONSWIDGET_H
