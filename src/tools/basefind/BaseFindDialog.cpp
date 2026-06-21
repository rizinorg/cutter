#include "BaseFindDialog.h"

#include "BaseFindSearchDialog.h"
#include "ui_BaseFindDialog.h"

#include <core/Cutter.h>
#include <rz_th.h>

BaseFindDialog::BaseFindDialog(QWidget *parent) : QDialog(parent), ui(new Ui::BaseFindDialog)
{
    ui->setupUi(this);
    setWindowFlags(windowFlags() & (~Qt::WindowContextHelpButtonHint));

    // Fill in N-thread Combo
    const RzThreadNCores nCores = rz_th_physical_core_number();
    ui->nCoresCombo->clear();
    for (size_t i = nCores; i > 0; i--) {
        if (nCores == i) {
            ui->nCoresCombo->addItem("All Cores");
            continue;
        }
        ui->nCoresCombo->addItem(QString::number(i));
    }

    ui->startAddressEdit->setText(Core()->getConfig("basefind.search.start"));
    ui->endAddressEdit->setText(Core()->getConfig("basefind.search.end"));
    ui->alignmentEdit->setText(Core()->getConfig("basefind.alignment"));
    ui->minStrLenEdit->setValue(Core()->getConfigut64("basefind.min.string"));
    ui->minScoreEdit->setValue(Core()->getConfigut64("basefind.min.score"));

    const size_t selectedNCores = Core()->getConfigut64("basefind.max.threads");
    if (selectedNCores < nCores && selectedNCores > 0) {
        ui->nCoresCombo->setCurrentIndex(nCores - selectedNCores);
    }

    connect(ui->buttonBox, &QDialogButtonBox::accepted, this, &BaseFindDialog::onButtonBoxAccepted);
    connect(ui->buttonBox, &QDialogButtonBox::rejected, this, &BaseFindDialog::onButtonBoxRejected);
}

BaseFindDialog::~BaseFindDialog() {}

RzThreadNCores BaseFindDialog::getNCores() const
{
    const RzThreadNCores nCores = rz_th_physical_core_number();
    return static_cast<RzThreadNCores>(nCores - ui->nCoresCombo->currentIndex());
}

ut32 BaseFindDialog::getPointerSize() const
{
    auto index = ui->pointerSizeCombo->currentIndex();
    const QString value = ui->pointerSizeCombo->itemText(index);
    return value.toULong(nullptr, 0);
}

RVA BaseFindDialog::getStartAddress() const
{
    const QString value = ui->startAddressEdit->text();
    return value.toULongLong(nullptr, 0);
}

RVA BaseFindDialog::getEndAddress() const
{
    const QString value = ui->endAddressEdit->text();
    return value.toULongLong(nullptr, 0);
}

RVA BaseFindDialog::getAlignment() const
{
    const QString value = ui->alignmentEdit->text();
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

void BaseFindDialog::onButtonBoxAccepted()
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

    auto *bfs = new BaseFindSearchDialog(parentWidget());
    bfs->show(&options);
}

void BaseFindDialog::onButtonBoxRejected() {}
