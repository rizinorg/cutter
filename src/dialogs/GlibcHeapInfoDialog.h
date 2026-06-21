#ifndef HEAPINFODIALOG_H
#define HEAPINFODIALOG_H

#include "CutterDescriptions.h" // IWYU pragma: keep

#include <QDialog>

#include <memory>

namespace Ui {
class GlibcHeapInfoDialog;
}

/**
 * @brief Dialog for inspecting and editing the metadata of a single glibc heap chunk
 */
class GlibcHeapInfoDialog : public QDialog
{
    Q_OBJECT

public:
    explicit GlibcHeapInfoDialog(RVA offset, QString status, QWidget *parent = nullptr);
    ~GlibcHeapInfoDialog();

private slots:
    void saveChunkInfo();

private:
    std::unique_ptr<Ui::GlibcHeapInfoDialog> ui;
    void updateFields();
    RVA offset;
    QString status;
};

#endif // HEAPINFODIALOG_H
