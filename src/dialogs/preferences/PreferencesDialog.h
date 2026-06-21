#ifndef PREFERENCESDIALOG_H
#define PREFERENCESDIALOG_H

#include "core/MainWindow.h"

#include <QDialog>

#include <memory>

class QTreeWidgetItem;

namespace Ui {
class PreferencesDialog;
}

/**
 * @brief Main Dialog that contains preference settings
 */
class PreferencesDialog : public QDialog
{
    Q_OBJECT

public:
    enum class Section : ut8 { Appearance, Disassembly };

    explicit PreferencesDialog(MainWindow *parent = nullptr);
    ~PreferencesDialog();

    void showSection(Section section);
    MainWindow *getMainWindow();

public slots:
    void changePage(QTreeWidgetItem *current, QTreeWidgetItem *previous);

private:
    std::unique_ptr<Ui::PreferencesDialog> ui;
    void chooseThemeIcons();
    MainWindow *mainWindow;
};

#endif // PREFERENCESDIALOG_H
