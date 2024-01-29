#include "BaseFindDialog.h"
#include "ui_BaseFindDialog.h"

#include "BaseFindSearchDialog.h"

#include <core/Cutter.h>
#include <rz_th.h>

BaseFindDialog::BaseFindDialog(QWidget *parent) : QDialog(parent), ui(new Ui::BaseFindDialog)
{
    ui->setupUi(this);
    setWindowFlags(windowFlags() & (~Qt::WindowContextHelpButtonHint));

    // Fill in N-thread Combo
    size_t n_cores = rz_th_physical_core_number();
    ui->nCoresCombo->clear();
    for (size_t i = n_cores; i > 0; i--) {
        if (n_cores == i) {
            ui->nCoresCombo->addItem(tr("All Cores"));
            continue;
        }
        ui->nCoresCombo->addItem(QString::number(i));
    }

    ui->startAddressEdit->setText(Core()->getConfig("basefind.search.start"));
    ui->endAddressEdit->setText(Core()->getConfig("basefind.search.end"));
    ui->alignmentEdit->setText(Core()->getConfig("basefind.alignment"));
    ui->minStrLenEdit->setValue(Core()->getConfigut64("basefind.min.string"));
    ui->minScoreEdit->setValue(Core()->getConfigut64("basefind.min.score"));

    size_t selected_n_cores = Core()->getConfigut64("basefind.max.threads");
    if (selected_n_cores < n_cores && selected_n_cores > 0) {
        ui->nCoresCombo->setCurrentIndex(n_cores - selected_n_cores);
    }
}

BaseFindDialog::~BaseFindDialog() {}

size_t BaseFindDialog::getNCores() const
{
    size_t n_cores = rz_th_physical_core_number();
    return n_cores - ui->nCoresCombo->currentIndex();
}

ut32 BaseFindDialog::getPointerSize() const
{
    auto index = ui->pointerSizeCombo->currentIndex();
    QString value = ui->pointerSizeCombo->itemText(index);
    return value.toULong(nullptr, 0);
}

RVA BaseFindDialog::getStartAddress() const
{
    QString value = ui->startAddressEdit->text();
    return value.toULongLong(nullptr, 0);
}

RVA BaseFindDialog::getEndAddress() const
{
    QString value = ui->endAddressEdit->text();
    return value.toULongLong(nullptr, 0);
}

RVA BaseFindDialog::getAlignment() const
{
    QString value = ui->alignmentEdit->text();
    return value.toULongLong(nullptr, 0);
}

ut32 BaseFindDialog::getMinStrLen() const
{
    return ui->minStrLenEdit->value();
}

ut32 BaseFindDialog::getMinScore() const
{
    return ui->minScoreEdit->value();
}

void BaseFindDialog::on_buttonBox_accepted()
{
    RzBaseFindOpt options = {};
    options.max_threads = getNCores();
    options.pointer_size = getPointerSize();
    options.start_address = getStartAddress();
    options.end_address = getEndAddress();
    options.alignment = getAlignment();
    options.min_score = getMinScore();
    options.min_string_len = getMinStrLen();
    options.callback = nullptr;
    options.user = nullptr;

    BaseFindSearchDialog *bfs = new BaseFindSearchDialog(parentWidget());
    bfs->show(&options);
}

void BaseFindDialog::on_buttonBox_rejected() {}
