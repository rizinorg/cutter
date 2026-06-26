#ifndef VERSIONINFODIALOG_H
#define VERSIONINFODIALOG_H

#include "CutterCommon.h" // IWYU pragma: keep

#include <QDialog>
#include <QTreeWidget>

#include <memory>

namespace Ui {
class VersionInfoDialog;
}

class VersionInfoDialog : public QDialog
{
    Q_OBJECT

public:
    explicit VersionInfoDialog(QWidget *parent = nullptr);
    ~VersionInfoDialog();

    enum Column : ut8 { KeyColumn = 0, ValueColumn = 1 };

private slots:
    void copyTreeWidgetSelection(QTreeWidget *t);
    void clearSelectionOnClose();

protected:
    QMenu *contextMenu = nullptr;
    QAction *copyActionLeftTreewidget = nullptr;
    QAction *copyActionRightTreewidget = nullptr;
    QAction *selAllActionLeftTreewidget = nullptr;
    QAction *selAllActionRightTreewidget = nullptr;

    void contextMenuEvent(QContextMenuEvent *event) override;
    void onButtonBoxRejected();

    /**
     * @fn AboutDialog::on_copyVersionInfoButton_clicked()
     *
     * @brief Copies the table values to Clipboard.
     */
    void onCopyVersionInfoButtonClicked();

private:
    std::unique_ptr<Ui::VersionInfoDialog> ui;

    void fillVersionInfo();
};

#endif // VERSIONINFODIALOG_H
