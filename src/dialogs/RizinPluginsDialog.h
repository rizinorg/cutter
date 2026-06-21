#ifndef PLUGINSDIALOG_H
#define PLUGINSDIALOG_H

#include <QAbstractTableModel>
#include <QDialog>

#include <memory>

namespace Ui {
class RizinPluginsDialog;
}

/**
 * @brief Dialog for listing and describing all loaded Rizin plugins, categorized by their subsystem
 * (Bin, IO, Core, Asm)
 */
class RizinPluginsDialog : public QDialog
{
    Q_OBJECT

public:
    explicit RizinPluginsDialog(QWidget *parent = nullptr);
    ~RizinPluginsDialog();

private:
    std::unique_ptr<Ui::RizinPluginsDialog> ui;
};

#endif // PLUGINSDIALOG_H
