#ifndef VERSIONINFODIALOG_H
#define VERSIONINFODIALOG_H

#include <QDialog>
#include <memory>

#include "core/Cutter.h"

namespace Ui {
class VersionInfoDialog;
}

class VersionInfoDialog : public QDialog
{
    Q_OBJECT

public:
    explicit VersionInfoDialog(QWidget *parent = nullptr);
    ~VersionInfoDialog();

private slots:
    void on_buttonBox_rejected();

    /**
     * @fn AboutDialog::on_copyVersionInfoButton_clicked()
     *
     * @brief Copies the table values to Clipboard.
     */
    void on_copyVersionInfoButton_clicked();

private:
    std::unique_ptr<Ui::VersionInfoDialog> ui;
    CutterCore *core;

    void fillVersionInfo();
};

#endif // VERSIONINFODIALOG_H
