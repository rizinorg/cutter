#ifndef PLUGINSDIALOG_H
#define PLUGINSDIALOG_H

#include <QDialog>
#include <QAbstractTableModel>

#include "core/Cutter.h"

namespace Ui {
class R2PluginsDialog;
}

class R2PluginsDialog : public QDialog
{
    Q_OBJECT

public:
    explicit R2PluginsDialog(QWidget *parent = nullptr);
    ~R2PluginsDialog();

private:
    Ui::R2PluginsDialog *ui;
};

#endif // PLUGINSDIALOG_H
