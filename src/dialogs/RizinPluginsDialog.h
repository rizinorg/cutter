#ifndef PLUGINSDIALOG_H
#define PLUGINSDIALOG_H

#include <QDialog>
#include <QAbstractTableModel>

#include "core/Cutter.h"

namespace Ui {
class RizinPluginsDialog;
}

class RizinPluginsDialog : public QDialog
{
    Q_OBJECT

public:
    explicit RizinPluginsDialog(QWidget *parent = nullptr);
    ~RizinPluginsDialog();

private:
    Ui::RizinPluginsDialog *ui;
};

#endif // PLUGINSDIALOG_H
