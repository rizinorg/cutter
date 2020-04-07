#ifndef MAPFILEDIALOG_H
#define MAPFILEDIALOG_H

#include <QDialog>
#include <memory>
#include "core/Cutter.h"

namespace Ui {
class MapFileDialog;
}

class MapFileDialog : public QDialog
{
    Q_OBJECT

public:
    explicit MapFileDialog(QWidget *parent = nullptr);
    ~MapFileDialog();

private slots:
    void on_selectFileButton_clicked();
    void on_buttonBox_accepted();
    void on_buttonBox_rejected();

private:
    std::unique_ptr<Ui::MapFileDialog> ui;
};

#endif // MAPFILEDIALOG_H
