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

    enum Column { KeyColumn = 0, ValueColumn = 1 };

private slots:
    void CopyTreeWidgetSelection(QTreeWidget *t);
    void clearSelectionOnClose();

protected:
    QMenu *contextMenu = nullptr;
    QAction *copyActionLeftTreewidget = nullptr;
    QAction *copyActionRightTreewidget = nullptr;
    QAction *selAllActionLeftTreewidget = nullptr;
    QAction *selAllActionRightTreewidget = nullptr;

    void contextMenuEvent(QContextMenuEvent *event) override;
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
