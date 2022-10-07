#include "core/Cutter.h"
#include "BinDiffDialog.h"

#include "ui_BinDiffDialog.h"
#include "common/Configuration.h"
#include "common/BugReporting.h"

#include <QUrl>
#include <QTimer>
#include <QEventLoop>
#include <QJsonObject>
#include <QProgressBar>
#include <QProgressDialog>
#include <UpdateWorker.h>
#include <QtNetwork/QNetworkRequest>
#include <QtNetwork/QNetworkAccessManager>

#include "CutterConfig.h"

typedef struct {
    QColor perfect;
    QColor partial;
    QColor none;
} DiffColors;

inline QString doubleToString(double number)
{
    return QString::asprintf("%.6f", number);
}

inline QTableWidgetItem *newColoredCell(QString text, double similarity, const DiffColors *colors) {
    auto item = new QTableWidgetItem(text);
    if (similarity >= 1.0) {
        item->setBackground(colors->perfect);
    } else if (similarity >= RZ_ANALYSIS_SIMILARITY_THRESHOLD) {
        item->setBackground(colors->partial);
    } else {
        item->setBackground(colors->none);
    }
    item->setForeground(Qt::black);
    return item;
}

BinDiffDialog::BinDiffDialog(RzAnalysisMatchResult *result, QWidget *parent) : QDialog(parent), ui(new Ui::BinDiffDialog)
{
    ui->setupUi(this);
    setWindowFlags(windowFlags() & (~Qt::WindowContextHelpButtonHint));

    void *ptr;
    RzListIter *iter = nullptr;
    RzAnalysisMatchPair *pair = nullptr;
    const RzAnalysisFunction *fcn_a = nullptr, *fcn_b = nullptr;

    const DiffColors colors = {
        .perfect = Config()->getColor("gui.match.perfect"),
        .partial = Config()->getColor("gui.match.partial"),
        .none = Config()->getColor("gui.match.none"),
    };

    size_t n_raws = rz_list_length(result->matches);
    n_raws += rz_list_length(result->unmatch_a);
    n_raws += rz_list_length(result->unmatch_b);

    QStringList tableHeader = {
        tr("name"), tr("size"), tr("addr"),
        tr("type"), tr("similarity"),
        tr("addr"), tr("size"), tr("name")
    };

    ui->resultTable->setRowCount(n_raws);
    ui->resultTable->setColumnCount(8);
    ui->resultTable->setHorizontalHeaderLabels(tableHeader);
    ui->resultTable->verticalHeader()->setVisible(false);
    ui->resultTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);

    n_raws = 0;
    rz_list_foreach(result->matches, iter, ptr) {
        pair = static_cast<RzAnalysisMatchPair *>(ptr);
        fcn_a = static_cast<const RzAnalysisFunction *>(pair->pair_a);
        fcn_b = static_cast<const RzAnalysisFunction *>(pair->pair_b);

        QString size_a = RzSizeString(rz_analysis_function_realsize(fcn_a));
        QString size_b = RzSizeString(rz_analysis_function_realsize(fcn_b));
        QString addr_a = RzAddressString(fcn_a->addr);
        QString addr_b = RzAddressString(fcn_b->addr);
        QString simtype =  RZ_ANALYSIS_SIMILARITY_TYPE_STR(pair->similarity);
        QString similarity = doubleToString(pair->similarity);

        ui->resultTable->setItem(n_raws, ColumnSourceName, newColoredCell(fcn_a->name, pair->similarity, &colors));
        ui->resultTable->setItem(n_raws, ColumnSourceSize, newColoredCell(size_a, pair->similarity, &colors));
        ui->resultTable->setItem(n_raws, ColumnSourceAddr, newColoredCell(addr_a, pair->similarity, &colors));
        ui->resultTable->setItem(n_raws, ColumnSimilarityType, newColoredCell(simtype, pair->similarity, &colors));
        ui->resultTable->setItem(n_raws, ColumnSimilarityNum, newColoredCell(similarity, pair->similarity, &colors));
        ui->resultTable->setItem(n_raws, ColumnMatchAddr, newColoredCell(addr_b, pair->similarity, &colors));
        ui->resultTable->setItem(n_raws, ColumnMatchSize, newColoredCell(size_b, pair->similarity, &colors));
        ui->resultTable->setItem(n_raws, ColumnMatchName, newColoredCell(fcn_b->name, pair->similarity, &colors));
        n_raws++;
    }

    rz_list_foreach(result->unmatch_a, iter, ptr) {
        fcn_a = static_cast<RzAnalysisFunction *>(ptr);
        QString size_a = RzSizeString(rz_analysis_function_realsize(fcn_a));
        QString addr_a = RzAddressString(fcn_a->addr);
        ui->resultTable->setItem(n_raws, ColumnSourceName, newColoredCell(fcn_a->name, 0.0, &colors));
        ui->resultTable->setItem(n_raws, ColumnSourceSize, newColoredCell(size_a, 0.0, &colors));
        ui->resultTable->setItem(n_raws, ColumnSourceAddr, newColoredCell(addr_a, 0.0, &colors));
        ui->resultTable->setItem(n_raws, ColumnSimilarityType, newColoredCell(RZ_ANALYSIS_SIMILARITY_UNLIKE_STR, 0.0, &colors));
        ui->resultTable->setItem(n_raws, ColumnSimilarityNum, newColoredCell("0.000000", 0.0, &colors));
        ui->resultTable->setItem(n_raws, ColumnMatchAddr, newColoredCell("------", 0.0, &colors));
        ui->resultTable->setItem(n_raws, ColumnMatchSize, newColoredCell("------", 0.0, &colors));
        ui->resultTable->setItem(n_raws, ColumnMatchName, newColoredCell("------", 0.0, &colors));
        n_raws++;
    }

    rz_list_foreach(result->unmatch_b, iter, ptr) {
        fcn_b = static_cast<RzAnalysisFunction *>(ptr);
        QString size_b = RzSizeString(rz_analysis_function_realsize(fcn_b));
        QString addr_b = RzAddressString(fcn_b->addr);
        ui->resultTable->setItem(n_raws, ColumnSourceName, newColoredCell("------", 0.0, &colors));
        ui->resultTable->setItem(n_raws, ColumnSourceSize, newColoredCell("------", 0.0, &colors));
        ui->resultTable->setItem(n_raws, ColumnSourceAddr, newColoredCell("------", 0.0, &colors));
        ui->resultTable->setItem(n_raws, ColumnSimilarityType, newColoredCell(RZ_ANALYSIS_SIMILARITY_UNLIKE_STR, 0.0, &colors));
        ui->resultTable->setItem(n_raws, ColumnSimilarityNum, newColoredCell("0.000000", 0.0, &colors));
        ui->resultTable->setItem(n_raws, ColumnMatchAddr, newColoredCell(fcn_b->name, 0.0, &colors));
        ui->resultTable->setItem(n_raws, ColumnMatchSize, newColoredCell(size_b, 0.0, &colors));
        ui->resultTable->setItem(n_raws, ColumnMatchName, newColoredCell(addr_b, 0.0, &colors));
        n_raws++;
    }
}

BinDiffDialog::~BinDiffDialog() {}

void BinDiffDialog::on_buttonBox_rejected()
{
    close();
}

