#ifndef BASEFIND_SEARCH_DIALOG_H
#define BASEFIND_SEARCH_DIALOG_H

#include <QDialog>
#include <QListWidgetItem>
#include <QProgressBar>
#include <memory>

#include <core/Cutter.h>

namespace Ui {
class BaseFindSearchDialog;
}

class BaseFindSearchDialog : public QDialog
{
    Q_OBJECT

public:
    explicit BaseFindSearchDialog(QWidget *parent = nullptr);
    ~BaseFindSearchDialog();

    void show(RzBaseFindOpt *opts);

public slots:
    void onProgress(BasefindCoreStatusDescription status);
    void onCompletion();

signals:
    void cancelSearch();

private slots:
    void on_buttonBox_rejected();

private:
    std::vector<QProgressBar *> bars;
    std::unique_ptr<Basefind> basefind;
    std::unique_ptr<Ui::BaseFindSearchDialog> ui;
};

#endif // BASEFIND_SEARCH_DIALOG_H
