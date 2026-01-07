#ifndef PROFILEDIRECTIVESDIALOG_H
#define PROFILEDIRECTIVESDIALOG_H

#include <QDialog>
#include <QStandardItemModel>
#include <QSortFilterProxyModel>
#include <memory>

namespace Ui {
class ProfileDirectivesDialog;
}

/**
 * @brief Dialog displaying a searchable list of available RzRun profile directives
 */
class ProfileDirectivesDialog : public QDialog
{
    Q_OBJECT

public:
    explicit ProfileDirectivesDialog(QWidget *parent = nullptr);
    ~ProfileDirectivesDialog();

private:
    std::unique_ptr<Ui::ProfileDirectivesDialog> ui;
    QStandardItemModel *model;
    QSortFilterProxyModel *proxyModel;

    /**
     * @brief Adds a directive entry to the list model
     * @param key The directive name (e.g., "aslr")
     * @param description Explanation of what the directive does
     */
    void addDirective(const QString &key, const QString &description);
};

#endif
