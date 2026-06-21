#ifndef SYMBOLSOPTIONSWIDGET_H
#define SYMBOLSOPTIONSWIDGET_H

#include <QDialog>
#include <QWidget>

#include <memory>

class MainWindow;

class PreferencesDialog;

namespace Ui {
class SymbolsOptionsWidget;
}

/**
 * @brief Contains configurable options related to symbols
 */
class SymbolsOptionsWidget : public QDialog
{
    Q_OBJECT

public:
    explicit SymbolsOptionsWidget(PreferencesDialog *parent = nullptr);
    ~SymbolsOptionsWidget();

private slots:
    void pdbSelectButtonClicked();

    void updateSymbolsOptions();

private:
    MainWindow *mainWindow;
    std::unique_ptr<Ui::SymbolsOptionsWidget> ui;
    void updateDebuginfodLayout();
    void reanalyze();
};

#endif // SYMBOLSOPTIONSWIDGET_H
