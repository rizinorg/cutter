/**
 * @file test_OpenFile.cpp
 * @brief Opening a binary: NewFileDialog, then InitialOptionsDialog, then analysis.
 */

#include "CutterApplication.h"
#include "Fixtures.h"
#include "TestEnv.h"
#include "UiHelpers.h"
#include "common/AsyncTask.h"
#include "core/MainWindow.h"
#include "dialogs/InitialOptionsDialog.h"
#include "dialogs/NewFileDialog.h"

#include <QComboBox>
#include <QDir>
#include <QFileInfo>
#include <QPointer>
#include <QSlider>
#include <QTest>

namespace {

constexpr int analysisTimeoutMs = 60000;
constexpr int autoAnalysisLevel = 1;

/**
 * @brief Enters path in the new file dialog and opens it.
 */
bool submitFile(NewFileDialog *dialog, const QString &path)
{
    return dialog && CutterTest::setLineEditText(dialog, "newFileEdit", path)
            && CutterTest::clickButton(dialog, "loadFileButton");
}

} // namespace

class TestOpenFile : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void init();
    void cleanup();

    void reselectAfterCancelingOptions();
    void errorClearedAfterValidSelection();
    void chosenOptionsReachAnalysis();

private:
    QString helloPath;
    QString crackmePath;
};

void TestOpenFile::initTestCase()
{
    CUTTER_REQUIRE_FIXTURE(helloPath, CutterTest::Fixtures::helloX64());
    CUTTER_REQUIRE_FIXTURE(crackmePath, CutterTest::Fixtures::crackmeX86());
}

void TestOpenFile::init()
{
    QTRY_VERIFY(CutterTest::findVisibleTopLevel<NewFileDialog>());
}

void TestOpenFile::cleanup()
{
    if (auto *optionsDialog = CutterTest::findVisibleTopLevel<InitialOptionsDialog>()) {
        CutterTest::clickButton(optionsDialog, "cancelButton");
    }
}

/**
 * @brief After cancelling the options dialog, another file can be chosen in the new file dialog.
 *
 * To reproduce:
 * 1. Choose a file and press Cancel in the options dialog.
 * 2. Choose another file in the new file dialog that opens: it was rejected.
 */
void TestOpenFile::reselectAfterCancelingOptions()
{
    CutterTest::MessageBoxWatcher messageBoxes;

    QPointer<NewFileDialog> newFileDialog = CutterTest::findVisibleTopLevel<NewFileDialog>();
    QVERIFY(submitFile(newFileDialog, helloPath));

    QPointer<InitialOptionsDialog> optionsDialog;
    QTRY_VERIFY((optionsDialog = CutterTest::findVisibleTopLevel<InitialOptionsDialog>()));
    QTRY_VERIFY(newFileDialog.isNull() || !newFileDialog->isVisible());

    QVERIFY(CutterTest::clickButton(optionsDialog, "cancelButton"));

    QTRY_VERIFY(optionsDialog.isNull() || !optionsDialog->isVisible());
    QTRY_VERIFY((newFileDialog = CutterTest::findVisibleTopLevel<NewFileDialog>()));
    QVERIFY(!CutterTest::core()->getAsyncTaskManager()->getTasksRunning());
    QVERIFY(!CutterTest::mainWindow()->isVisible());
    QVERIFY(submitFile(newFileDialog, crackmePath));

    QTRY_VERIFY(CutterTest::findVisibleTopLevel<InitialOptionsDialog>());
    QCOMPARE(messageBoxes.seenTexts(), QStringList());
}

/**
 * @brief The error about a missing file goes away once a valid file is chosen.
 *
 * To reproduce:
 * 1. Enter a path that doesn't exist in the new file dialog.
 * 2. Choose a valid file: the error kept appearing.
 */
void TestOpenFile::errorClearedAfterValidSelection()
{
    const QString missingPath = QDir(QDir::tempPath()).filePath("cutter-test-missing-binary");
    QVERIFY(!QFileInfo::exists(missingPath));

    CutterTest::MessageBoxWatcher messageBoxes;

    QPointer<NewFileDialog> newFileDialog = CutterTest::findVisibleTopLevel<NewFileDialog>();
    QVERIFY(submitFile(newFileDialog, missingPath));
    QTRY_COMPARE(messageBoxes.seenTexts().size(), 1);
    QVERIFY(!newFileDialog.isNull() && newFileDialog->isVisible());
    QVERIFY(!CutterTest::findVisibleTopLevel<InitialOptionsDialog>());

    QVERIFY(submitFile(newFileDialog, helloPath));
    QTRY_VERIFY(CutterTest::findVisibleTopLevel<InitialOptionsDialog>());
    QCOMPARE(messageBoxes.seenTexts().size(), 1);
}

/**
 * @brief The architecture, bits, endianness and analysis level chosen reach the analysis.
 */
void TestOpenFile::chosenOptionsReachAnalysis()
{
    QVERIFY(submitFile(CutterTest::findVisibleTopLevel<NewFileDialog>(), helloPath));
    InitialOptionsDialog *optionsDialog = nullptr;
    QTRY_VERIFY((optionsDialog = CutterTest::findVisibleTopLevel<InitialOptionsDialog>()));

    auto *arch = optionsDialog->findChild<QComboBox *>("archComboBox");
    auto *bits = optionsDialog->findChild<QComboBox *>("bitsComboBox");
    auto *endianness = optionsDialog->findChild<QComboBox *>("endiannessComboBox");
    auto *level = optionsDialog->findChild<QSlider *>("analysisSlider");
    QVERIFY(arch && bits && endianness && level);

    const int x86 = arch->findData("x86");
    const int bits32 = bits->findText("32");
    QVERIFY(x86 >= 0);
    QVERIFY(bits32 >= 0);
    arch->setCurrentIndex(x86);
    bits->setCurrentIndex(bits32);
    endianness->setCurrentIndex(1);
    level->setValue(autoAnalysisLevel);

    QVERIFY(CutterTest::clickButton(optionsDialog, "okButton"));

    auto *app = qobject_cast<CutterApplication *>(QCoreApplication::instance());
    QVERIFY(app);
    const InitialOptions options = app->getInitialOptions();
    QVERIFY(options.filename.endsWith(QDir::toNativeSeparators(helloPath)));
    QCOMPARE(options.arch, QString("x86"));
    QCOMPARE(options.bits, 32);
    QCOMPARE(options.endian, InitialOptions::Endianness::Little);
    QCOMPARE(options.analysisCmd.size(), 1);
    QCOMPARE(options.analysisCmd.first().command, QString("aaa"));

    QTRY_VERIFY_WITH_TIMEOUT(CutterTest::mainWindow()->isVisible(), analysisTimeoutMs);
}

CUTTER_APP_TEST_MAIN(TestOpenFile)
#include "test_OpenFile.moc"
