/**
 * @file test_IOModes.cpp
 * @brief Cutter's IO modes: read-only, cache and write.
 */

#include "Fixtures.h"
#include "TestEnv.h"

#include <QCryptographicHash>
#include <QFile>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QTest>

namespace {

/**
 * @brief The byte with every bit flipped, so it always differs.
 */
QByteArray flippedByte(const QByteArray &original)
{
    return QByteArray(1, static_cast<char>(original.at(0) ^ 0xff));
}

QString hexOf(const QByteArray &bytes)
{
    return QString::fromLatin1(bytes.toHex());
}

} // namespace

class TestIOModes : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void init();
    void cleanup();

    void cacheModeEditLeavesFileUntouched();
    void commitWriteCacheWritesFile();
    void writeModeEditPersists();
    void modeSwitchesKeepCacheAndWriteExclusive();

private:
    QByteArray fileHash() const;
    void restoreFile() const;
    static bool inMode(bool cache, bool write);

    QTemporaryDir tempDir;
    QString filePath;
    QByteArray originalBytes;
    QByteArray originalHash;
    RVA address = RVA_INVALID;
};

void TestIOModes::initTestCase()
{
    CUTTER_REQUIRE_FIXTURE(const QString fixture, CutterTest::Fixtures::helloX64());
    QVERIFY(tempDir.isValid());
    filePath = tempDir.filePath("io-modes-fixture");
    QVERIFY(QFile::copy(fixture, filePath));
    QVERIFY(QFile::setPermissions(filePath, QFile::ReadOwner | QFile::WriteOwner));

    QFile file(filePath);
    QVERIFY(file.open(QIODevice::ReadOnly));
    originalBytes = file.readAll();
    originalHash = QCryptographicHash::hash(originalBytes, QCryptographicHash::Sha256);
    QVERIFY(CutterTest::openFile(filePath));
    address = CutterTest::core()->getOffset();
    QVERIFY(address != RVA_INVALID);
}

void TestIOModes::init()
{
    QVERIFY(inMode(false, false));
    QCOMPARE(fileHash(), originalHash);
}

void TestIOModes::cleanup()
{
    CutterCore *core = CutterTest::core();
    core->resetWriteCache();
    core->setWriteMode(false);
    core->setIOCache(false);
    if (fileHash() != originalHash) {
        restoreFile();
    }
}

void TestIOModes::restoreFile() const
{
    QFile file(filePath);
    if (file.open(QIODevice::ReadWrite)) {
        file.write(originalBytes);
    }
}

QByteArray TestIOModes::fileHash() const
{
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        return {};
    }
    return QCryptographicHash::hash(file.readAll(), QCryptographicHash::Sha256);
}

bool TestIOModes::inMode(bool cache, bool write)
{
    CutterCore *core = CutterTest::core();
    return core->isIOCacheEnabled() == cache && core->isWriteModeEnabled() == write;
}

/**
 * @brief An edit in cache mode is visible in Cutter but doesn't touch the file.
 */
void TestIOModes::cacheModeEditLeavesFileUntouched()
{
    CutterCore *core = CutterTest::core();
    core->setIOCache(true);
    QVERIFY(inMode(true, false));

    const QByteArray original = core->ioRead(address, 1);
    const QByteArray patched = flippedByte(original);

    core->editBytes(address, hexOf(patched));
    QCOMPARE(core->ioRead(address, 1), patched);

    QCOMPARE(fileHash(), originalHash);
    core->resetWriteCache();
    QCOMPARE(core->ioRead(address, 1), original);
}

/**
 * @brief Committing the cache writes the edit to the file.
 *
 * To reproduce:
 * 1. File → Set mode → Cache mode, and change a byte in the Hexdump.
 * 2. File → Commit changes: a heap use after free under ASan.
 */
void TestIOModes::commitWriteCacheWritesFile()
{
    CutterCore *core = CutterTest::core();
    core->setIOCache(true);

    const QByteArray original = core->ioRead(address, 1);
    core->editBytes(address, hexOf(flippedByte(original)));
    core->commitWriteCache();
    QVERIFY(fileHash() != originalHash);
    QVERIFY(inMode(true, false));
}

/**
 * @brief An edit in write mode is written to the file.
 */
void TestIOModes::writeModeEditPersists()
{
    CutterCore *core = CutterTest::core();
    core->setWriteMode(true);
    QVERIFY(inMode(false, true));

    const QByteArray original = core->ioRead(address, 1);
    core->editBytes(address, hexOf(flippedByte(original)));
    QVERIFY(fileHash() != originalHash);
}

/**
 * @brief Cache mode and write mode are never on together, and each switch is signalled.
 */
void TestIOModes::modeSwitchesKeepCacheAndWriteExclusive()
{
    CutterCore *core = CutterTest::core();
    QSignalSpy modeSpy(core, &CutterCore::ioModeChanged);
    QSignalSpy cacheSpy(core, &CutterCore::ioCacheChanged);
    QSignalSpy writeSpy(core, &CutterCore::writeModeChanged);

    core->setIOCache(true);
    QVERIFY(inMode(true, false));
    QVERIFY(!modeSpy.isEmpty());
    QCOMPARE(cacheSpy.last().at(0).toBool(), true);

    modeSpy.clear();
    core->setWriteMode(true);
    QVERIFY(inMode(false, true));
    QVERIFY(!modeSpy.isEmpty());
    QCOMPARE(writeSpy.last().at(0).toBool(), true);
    QCOMPARE(cacheSpy.last().at(0).toBool(), false);

    modeSpy.clear();
    core->setIOCache(true);
    QVERIFY(inMode(true, false));
    QVERIFY(!modeSpy.isEmpty());
    QCOMPARE(writeSpy.last().at(0).toBool(), false);
    QCOMPARE(cacheSpy.last().at(0).toBool(), true);

    modeSpy.clear();
    core->setWriteMode(false);
    QVERIFY(inMode(false, false));
    QVERIFY(!modeSpy.isEmpty());
    QCOMPARE(cacheSpy.last().at(0).toBool(), false);

    QCOMPARE(fileHash(), originalHash);
}

CUTTER_CORE_TEST_MAIN(TestIOModes)
#include "test_IOModes.moc"
