#ifndef BINDIFFDIALOG_H
#define BINDIFFDIALOG_H

#include <QDialog>
#include <memory>
#include <QtNetwork/QNetworkReply>

namespace Ui {
class BinDiffDialog;
}

class BinDiffDialog : public QDialog
{
    Q_OBJECT

public:
    explicit BinDiffDialog(RzAnalysisMatchResult *result, QWidget *parent = nullptr);
    ~BinDiffDialog();

private slots:
    void on_buttonBox_rejected();

private:
    enum {
        ColumnSourceName = 0,
        ColumnSourceSize,
        ColumnSourceAddr,
        ColumnSimilarityType,
        ColumnSimilarityNum,
        ColumnMatchAddr,
        ColumnMatchSize,
        ColumnMatchName
    } BinDiffTableColumn;
    std::unique_ptr<Ui::BinDiffDialog> ui;
};

#endif // BINDIFFDIALOG_H
