#ifndef HEAPINFODIALOG_H
#define HEAPINFODIALOG_H

#include <QDialog>
#include "core/Cutter.h"

namespace Ui {
class HeapInfoDialog;
}

class HeapInfoDialog : public QDialog
{
    Q_OBJECT

public:
    explicit HeapInfoDialog(RVA offset, QWidget *parent = nullptr);
    ~HeapInfoDialog();

private:
    Ui::HeapInfoDialog *ui;
    void updateFields();
    RVA offset{};
};

#endif // HEAPINFODIALOG_H
