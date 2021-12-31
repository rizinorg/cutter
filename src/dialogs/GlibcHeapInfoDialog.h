#ifndef HEAPINFODIALOG_H
#define HEAPINFODIALOG_H

#include <QDialog>
#include "core/Cutter.h"

namespace Ui {
class GlibcHeapInfoDialog;
}

class GlibcHeapInfoDialog : public QDialog
{
    Q_OBJECT

public:
    explicit GlibcHeapInfoDialog(RVA offset, QString status, QWidget *parent = nullptr);
    ~GlibcHeapInfoDialog();

private slots:
    void saveChunkInfo();

private:
    Ui::GlibcHeapInfoDialog *ui;
    void updateFields();
    RVA offset;
    QString status;
};

#endif // HEAPINFODIALOG_H
