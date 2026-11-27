/**
 * @file test_CancelAnalysis.cpp
 * @brief Cancelling analysis from the progress dialog.
 *
 * Covers: AsyncTaskDialog, AnalysisTask.
 */

#include "TestBins.h"
#include "TestEnv.h"
#include "UiHelpers.h"

#include "core/MainWindow.h"
#include "dialogs/AsyncTaskDialog.h"
#include "dialogs/InitialOptionsDialog.h"
#include "dialogs/NewFileDialog.h"

#include <QDialogButtonBox>
#include <QPlainTextEdit>
#include <QPointer>
#include <QPushButton>
#include <QSlider>
#include <QTest>

namespace {

constexpr int experimentalAnalysisLevel = 2;
constexpr int analysisStartTimeoutMs = 60000;
constexpr int cancelTimeoutMs = 10000;

/**
 * @brief Whether the dialog's log shows that analysis began.
 *
 * Note: that this function is fragile and might break if something other than "Executing analysis"
 * is logged
 */
bool analysisStarted(AsyncTaskDialog *dialog)
{
    auto *log = dialog->findChild<QPlainTextEdit *>("logTextEdit");
    return log && log->toPlainText().contains("Executing analysis");
}

} // namespace

class TestCancelAnalysis : public QObject
{
    Q_OBJECT

private slots:
    void cancelDuringAnalysisDoesNotFreeze();
};

/**
 * @brief Cancel in the analysis progress dialog stops the analysis and leaves a usable window.
 *
 * Regression: pressing Cancel during analysis used to freeze Cutter.
 * Uses the experimental analysis level so analysis runs long enough to cancel, skips if it still
 * finishes first.
 */
void TestCancelAnalysis::cancelDuringAnalysisDoesNotFreeze()
{
    CUTTER_REQUIRE_BIN(const QString path, CutterTest::Bins::coreutilsLs());

    NewFileDialog *newFileDialog = nullptr;
    QTRY_VERIFY((newFileDialog = CutterTest::findVisibleTopLevel<NewFileDialog>()));
    QVERIFY(CutterTest::setLineEditText(newFileDialog, "newFileEdit", path));
    QVERIFY(CutterTest::clickButton(newFileDialog, "loadFileButton"));

    InitialOptionsDialog *optionsDialog = nullptr;
    QTRY_VERIFY((optionsDialog = CutterTest::findVisibleTopLevel<InitialOptionsDialog>()));
    auto *level = optionsDialog->findChild<QSlider *>("analysisSlider");
    QVERIFY(level);
    level->setValue(experimentalAnalysisLevel);
    QVERIFY(CutterTest::clickButton(optionsDialog, "okButton"));

    QPointer<AsyncTaskDialog> taskDialog;
    QTRY_VERIFY((taskDialog = CutterTest::findVisibleTopLevel<AsyncTaskDialog>()));
    QTRY_VERIFY_WITH_TIMEOUT(taskDialog.isNull() || analysisStarted(taskDialog),
                             analysisStartTimeoutMs);
    if (taskDialog.isNull()) {
        QSKIP("analysis finished before it could be cancelled");
    }

    auto *buttons = taskDialog->findChild<QDialogButtonBox *>("buttonBox");
    QVERIFY(buttons);
    QPushButton *cancel = buttons->button(QDialogButtonBox::Cancel);
    QVERIFY(cancel);
    cancel->click();

    AsyncTaskManager *tasks = CutterTest::core()->getAsyncTaskManager();
    QTRY_VERIFY_WITH_TIMEOUT(!tasks->getTasksRunning(), cancelTimeoutMs);
    QTRY_VERIFY(taskDialog.isNull() || !taskDialog->isVisible());
    QTRY_VERIFY(CutterTest::mainWindow()->isVisible()
                || CutterTest::findVisibleTopLevel<NewFileDialog>());
}

CUTTER_APP_TEST_MAIN(TestCancelAnalysis)
#include "test_CancelAnalysis.moc"
