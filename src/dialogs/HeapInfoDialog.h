#ifndef HEAPINFODIALOG_H
#define HEAPINFODIALOG_H

#include <QDialog>

namespace Ui {
class HeapInfoDialog;
}

class HeapInfoDialog : public QDialog
{
    Q_OBJECT

public:
    explicit HeapInfoDialog(QWidget *parent = nullptr);
    ~HeapInfoDialog();

private:
    Ui::HeapInfoDialog *ui;
};

#endif // HEAPINFODIALOG_H
