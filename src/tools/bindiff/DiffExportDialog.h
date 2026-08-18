#ifndef DIFFEXPORTDIALOG_H
#define DIFFEXPORTDIALOG_H

#include <QDialog>
#include <QStringLiteral>

#include <CutterDiff.h>
// AI Generated Start GPT GO
#include <QLabel>
#include <QProgressBar>
#include <QPushButton>
#include <QThread>

class DiffExportWorker : public QObject
{
    Q_OBJECT

public:
    DiffExportWorker(CutterDiff *cutterDiff, const QString &filePath)
        : cutterDiff(cutterDiff), filePath(filePath)
    {
    }

public slots:
    void exportDiff()
    {
        if (!cutterDiff) {
            emit finished(false, QStringLiteral("Invalid CutterDiff."));
            return;
        }

        const bool success = cutterDiff->saveDiffItemsToJson(filePath);

        if (success) {
            emit finished(true, {});
        } else {
            emit finished(false, QStringLiteral("Failed to save diff to \"%1\".").arg(filePath));
        }
    }

signals:
    void finished(bool success, const QString &error);

private:
    CutterDiff *cutterDiff;
    QString filePath;
};

class DiffExportDialog : public QDialog
{
    Q_OBJECT

public:
    explicit DiffExportDialog(CutterDiff *cutterDiff, QWidget *parent = nullptr);
    ~DiffExportDialog();

private slots:
    void selectFile();
    void startExport();

private:
    void setExporting(bool exporting);
    void showError(const QString &message);

    CutterDiff *cutterDiff;

    QString filePath;

    QLabel *statusLabel;
    QProgressBar *progressBar;
    QPushButton *fileButton;
    QPushButton *exportButton;
    QPushButton *cancelButton;
};

// AI Generated Ends GPT GO

#endif // DIFFEXPORTDIALOG_H
