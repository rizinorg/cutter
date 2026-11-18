#include "SubBinarySelectorDialog.h"

#include "ui_SubBinarySelectorDialog.h"

#include <QFileInfo>
#include <QHeaderView>
#include <QPushButton>
#include <QTableWidgetItem>

SubBinarySelectorDialog::SubBinarySelectorDialog(const QString &filename,
                                                 const QList<SubBinaryDescription> &subBinaries,
                                                 QWidget *parent)
    : QDialog(parent), ui(new Ui::SubBinarySelectorDialog), subBinaries(subBinaries)
{
    ui->setupUi(this);

    ui->infoLabel->setText(tr("<b>%1</b> contains %2 binaries. Select the one to load:")
                                   .arg(QFileInfo(filename).fileName().toHtmlEscaped())
                                   .arg(subBinaries.size()));

    ui->binariesTable->setRowCount(subBinaries.size());
    for (int row = 0; row < subBinaries.size(); row++) {
        const SubBinaryDescription &bin = subBinaries.at(row);
        ui->binariesTable->setItem(row, 0, new QTableWidgetItem(bin.arch));
        ui->binariesTable->setItem(row, 1, new QTableWidgetItem(QString::number(bin.bits)));
        ui->binariesTable->setItem(row, 2, new QTableWidgetItem(bin.machine));
        const QString offset = QStringLiteral("0x%1").arg(bin.offset, 0, 16);
        ui->binariesTable->setItem(row, 3, new QTableWidgetItem(offset));
        ui->binariesTable->setItem(row, 4, new QTableWidgetItem(QString::number(bin.size)));
    }
    ui->binariesTable->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
    ui->binariesTable->horizontalHeader()->setStretchLastSection(true);

    // Pre-select the first binary so OK always has something to load.
    if (!subBinaries.isEmpty()) {
        ui->binariesTable->selectRow(0);
    }

    // Double-clicking a row picks it, same as selecting it and pressing OK.
    connect(ui->binariesTable, &QTableWidget::cellDoubleClicked, this, &QDialog::accept);
    connect(ui->buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(ui->buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
}

SubBinarySelectorDialog::~SubBinarySelectorDialog() {}

SubBinaryDescription SubBinarySelectorDialog::getSelectedSubBinary() const
{
    int row = ui->binariesTable->currentRow();
    if (row < 0 || row >= subBinaries.size()) {
        row = 0;
    }
    return subBinaries.at(row);
}