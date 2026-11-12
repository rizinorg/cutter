/**
 * @file test_SeekHistory.cpp
 * @brief Seek and seek history signals of the core.
 */

#include "Fixtures.h"
#include "TestEnv.h"

#include <QSignalSpy>
#include <QTest>

namespace {

constexpr RVA seekDistance = 0x10;

using HistoryType = CutterCore::SeekHistoryType;

RVA seekedAddress(const QList<QVariant> &arguments)
{
    return arguments.at(0).toULongLong();
}

HistoryType historyType(const QList<QVariant> &arguments)
{
    return arguments.at(1).value<HistoryType>();
}

} // namespace

class TestSeekHistory : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();

    void seekNotifiesNewWithAddress();
    void seekPrevNextNotifyUndoRedo();
    void seekToCurrentAddressDoesNotNotify();
    void seekInvalidAddressIsIgnored();
    void seekSilentDoesNotNotify();

private:
    RVA first = RVA_INVALID;
    RVA second = RVA_INVALID;
};

void TestSeekHistory::initTestCase()
{
    CUTTER_REQUIRE_FIXTURE(const QString path, CutterTest::Fixtures::helloX64());

    QVERIFY(CutterTest::openFile(path));
    qRegisterMetaType<HistoryType>();

    first = CutterTest::core()->getOffset();
    QVERIFY(first != RVA_INVALID);
    second = first + seekDistance;
}

/**
 * @brief A seek emits seekChanged with the address and a new history entry.
 */
void TestSeekHistory::seekNotifiesNewWithAddress()
{
    CutterCore *core = CutterTest::core();
    core->seek(first);

    QSignalSpy spy(core, &CutterCore::seekChanged);

    core->seek(second);
    QCOMPARE(spy.count(), 1);
    const QList<QVariant> arguments = spy.takeFirst();
    QCOMPARE(seekedAddress(arguments), second);
    QCOMPARE(historyType(arguments), HistoryType::New);
    QCOMPARE(core->getOffset(), second);
}

/**
 * @brief Going back and forward emits seekChanged as undo and redo.
 */
void TestSeekHistory::seekPrevNextNotifyUndoRedo()
{
    CutterCore *core = CutterTest::core();
    core->seek(first);
    core->seek(second);

    QSignalSpy spy(core, &CutterCore::seekChanged);

    core->seekPrev();
    QCOMPARE(spy.count(), 1);
    QList<QVariant> arguments = spy.takeFirst();
    QCOMPARE(seekedAddress(arguments), first);
    QCOMPARE(historyType(arguments), HistoryType::Undo);
    QCOMPARE(core->getOffset(), first);

    core->seekNext();
    QCOMPARE(spy.count(), 1);
    arguments = spy.takeFirst();
    QCOMPARE(seekedAddress(arguments), second);
    QCOMPARE(historyType(arguments), HistoryType::Redo);
    QCOMPARE(core->getOffset(), second);
}

/**
 * @brief Seeking to the current address emits nothing.
 */
void TestSeekHistory::seekToCurrentAddressDoesNotNotify()
{
    CutterCore *core = CutterTest::core();
    core->seek(second);

    QSignalSpy spy(core, &CutterCore::seekChanged);

    core->seek(second);
    QCOMPARE(spy.count(), 0);
    QCOMPARE(core->getOffset(), second);
}

/**
 * @brief Seeking to RVA_INVALID neither moves nor emits.
 */
void TestSeekHistory::seekInvalidAddressIsIgnored()
{
    CutterCore *core = CutterTest::core();
    core->seek(first);

    QSignalSpy spy(core, &CutterCore::seekChanged);

    core->seek(RVA_INVALID);
    QCOMPARE(spy.count(), 0);
    QCOMPARE(core->getOffset(), first);
}

/**
 * @brief seekSilent moves without emitting seekChanged.
 */
void TestSeekHistory::seekSilentDoesNotNotify()
{
    CutterCore *core = CutterTest::core();
    core->seek(first);

    QSignalSpy spy(core, &CutterCore::seekChanged);

    core->seekSilent(second);
    QCOMPARE(spy.count(), 0);
    QCOMPARE(core->getOffset(), second);
}

CUTTER_CORE_TEST_MAIN(TestSeekHistory)
#include "test_SeekHistory.moc"
