/**
 * @file test_Shortcuts.cpp
 * @brief Keyboard shortcuts and the widget they reach.
 *
 * Covers: ShortcutManager and the shortcuts set up in the views.
 */

#include "TestBins.h"
#include "TestEnv.h"
#include "ViewHelpers.h"

#include "shortcuts/ShortcutManager.h"
#include "widgets/HexdumpWidget.h"

#include <QApplication>
#include <QClipboard>
#include <QTest>

namespace {

constexpr int copiedBytes = 4;

bool focusIsWithin(QWidget *widget)
{
    QWidget *focus = QApplication::focusWidget();
    return focus && (focus == widget || widget->isAncestorOf(focus));
}

} // namespace

class TestShortcuts : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void hexdumpCopyShortcutCopiesSelection();

private:
    MainWindow *main = nullptr;
};

void TestShortcuts::initTestCase()
{
    main = CutterTest::mainWindow();
    QVERIFY(main);
    main->activateWindow();
    QVERIFY(QTest::qWaitForWindowActive(main));
}

/**
 * @brief The copy shortcut in the Hexdump copies the selected bytes.
 *
 * Regression: the shortcut used to leave the clipboard unchanged.
 */
void TestShortcuts::hexdumpCopyShortcutCopiesSelection()
{
    auto *hexdump = CutterTest::findSynchronizedDock<HexdumpWidget>(main);
    QVERIFY(hexdump);
    hexdump->raiseMemoryWidget();
    auto *hex = hexdump->findChild<HexWidget *>();
    QVERIFY(hex);

    const QList<QKeySequence> keys = Shortcuts()->getKeySequences("Hex.copy");
    QVERIFY(!keys.isEmpty());

    const RVA start = CutterTest::core()->getOffset();
    hex->selectRange(start, start + copiedBytes - 1);
    const QString expected =
            QString::fromLatin1(CutterTest::core()->ioRead(start, copiedBytes).toHex());

    QApplication::clipboard()->clear();
    hex->setFocus(Qt::OtherFocusReason);
    QTRY_VERIFY(focusIsWithin(hex));

    QTest::keySequence(QApplication::focusWidget(), keys.first());
    QTRY_COMPARE(QApplication::clipboard()->text(), expected);
}

CUTTER_APP_TEST_MAIN_WITH_FILE(TestShortcuts, CutterTest::Bins::helloX64())
#include "test_Shortcuts.moc"
