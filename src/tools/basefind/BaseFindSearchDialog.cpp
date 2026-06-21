#include "BaseFindSearchDialog.h"

#include "BaseFindResultsDialog.h"
#include "ui_BaseFindSearchDialog.h"

#include <QFormLayout>
#include <QLabel>

#include <rz_th.h>

BaseFindSearchDialog::BaseFindSearchDialog(QWidget *parent)
    : QDialog(parent), basefind(new Basefind()), ui(new Ui::BaseFindSearchDialog)
{
    ui->setupUi(this);
    setWindowFlags(windowFlags() & (~Qt::WindowContextHelpButtonHint));
}

BaseFindSearchDialog::~BaseFindSearchDialog() {}

void BaseFindSearchDialog::show(RzBaseFindOpt *opts)
{
    const RzThreadNCores nCores = rz_th_physical_core_number();
    if (opts->max_threads > nCores || opts->max_threads < 1) {
        opts->max_threads = nCores;
    }

    auto *layout = new QFormLayout();
    ui->scrollAreaWidgetContents->setLayout(layout);
    for (ut32 i = 0; i < opts->max_threads; ++i) {
        const QString label = QString::asprintf("Core %u", i);
        auto *pbar = new QProgressBar(nullptr);
        layout->addRow(label, pbar);
        pbar->setRange(0, 100);
        bars.push_back(pbar);
    }

    if (!basefind->setOptions(opts)) {
        return;
    }

    connect(this, &BaseFindSearchDialog::cancelSearch, basefind.get(), &Basefind::cancel);
    connect(basefind.get(), &Basefind::progress, this, &BaseFindSearchDialog::onProgress);
    connect(basefind.get(), &Basefind::complete, this, &BaseFindSearchDialog::onCompletion);
    connect(ui->buttonBox, &QDialogButtonBox::rejected, this,
            &BaseFindSearchDialog::onButtonBoxRejected);

    basefind->start();
    this->QDialog::show();
}

void BaseFindSearchDialog::onProgress(BasefindCoreStatusDescription status)
{
    bars[status.index]->setValue(status.percentage);
}

void BaseFindSearchDialog::onCompletion()
{
    auto results = basefind->results();
    auto *table = new BaseFindResultsDialog(results, parentWidget());
    table->show();
    this->close();
}

void BaseFindSearchDialog::onButtonBoxRejected()
{
    emit cancelSearch();
}
