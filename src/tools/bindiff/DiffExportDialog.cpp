#include "DiffExportDialog.h"

#include <QFileDialog>
#include <QFileInfo>
#include <QHBoxLayout>
#include <QLabel>
#include <QMessageBox>
#include <QProgressBar>
#include <QPushButton>
#include <QThread>
#include <QVBoxLayout>

DiffExportDialog::DiffExportDialog(CutterDiff *cutterDiff, QWidget *parent)
    : QDialog(parent), cutterDiff(cutterDiff)
{
    setWindowTitle(tr("Export Diff"));
    setModal(true);

    auto *layout = new QVBoxLayout(this);

    statusLabel = new QLabel(tr("Select a file to export the diff."), this);
    layout->addWidget(statusLabel);

    auto *fileLayout = new QHBoxLayout();

    fileButton = new QPushButton(tr("Select File..."), this);
    fileLayout->addWidget(fileButton);

    layout->addLayout(fileLayout);

    progressBar = new QProgressBar(this);
    progressBar->setRange(0, 1);
    progressBar->setValue(0);
    progressBar->hide();

    layout->addWidget(progressBar);

    auto *buttonLayout = new QHBoxLayout();
    buttonLayout->addStretch();

    exportButton = new QPushButton(tr("Export"), this);
    exportButton->setEnabled(false);

    cancelButton = new QPushButton(tr("Cancel"), this);

    buttonLayout->addWidget(exportButton);
    buttonLayout->addWidget(cancelButton);

    layout->addLayout(buttonLayout);

    connect(fileButton, &QPushButton::clicked, this, &DiffExportDialog::selectFile);

    connect(exportButton, &QPushButton::clicked, this, &DiffExportDialog::startExport);

    connect(cancelButton, &QPushButton::clicked, this, &QDialog::reject);
}

DiffExportDialog::~DiffExportDialog() = default;

void DiffExportDialog::selectFile()
{
    QString selectedFile = QFileDialog::getSaveFileName(this, tr("Export Diff"), QString(),
                                                        tr("JSON files (*.json);;All files (*)"));

    if (selectedFile.isEmpty()) {
        return;
    }

    if (!selectedFile.endsWith(".json", Qt::CaseInsensitive)) {
        selectedFile += ".json";
    }

    const QFileInfo fileInfo(selectedFile);

    if (!fileInfo.dir().exists()) {
        showError(tr("The selected directory does not exist."));
        return;
    }

    filePath = selectedFile;

    statusLabel->setText(tr("Export to: %1").arg(filePath));

    exportButton->setEnabled(true);
}

void DiffExportDialog::startExport()
{
    if (filePath.isEmpty()) {
        return;
    }

    const QFileInfo fileInfo(filePath);

    if (!fileInfo.dir().exists()) {
        showError(tr("The selected directory no longer exists."));
        return;
    }

    setExporting(true);

    auto *thread = new QThread(this);
    auto *worker = new DiffExportWorker(cutterDiff, filePath);

    worker->moveToThread(thread);

    connect(thread, &QThread::started, worker, &DiffExportWorker::exportDiff);

    connect(worker, &DiffExportWorker::finished, this,
            [this, thread, worker](bool success, const QString &error) {
                thread->quit();

                if (success) {
                    progressBar->setRange(0, 1);
                    progressBar->setValue(1);

                    statusLabel->setText(tr("Diff exported successfully."));

                    QMetaObject::invokeMethod(this, [this]() { accept(); }, Qt::QueuedConnection);
                } else {
                    setExporting(false);
                    showError(error);
                }
            });

    connect(thread, &QThread::finished, worker, &QObject::deleteLater);

    connect(thread, &QThread::finished, thread, &QObject::deleteLater);

    thread->start();
}

void DiffExportDialog::setExporting(bool exporting)
{
    fileButton->setEnabled(!exporting);
    exportButton->setEnabled(!exporting);
    cancelButton->setEnabled(!exporting);

    if (exporting) {
        statusLabel->setText(tr("Exporting diff..."));

        // Indeterminate progress bar = busy/waiting state.
        progressBar->setRange(0, 0);
        progressBar->show();
    } else {
        progressBar->hide();
    }
}

void DiffExportDialog::showError(const QString &message)
{
    QMessageBox::critical(this, tr("Export Failed"), message);
}
