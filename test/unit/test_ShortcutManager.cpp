/**
 * @file test_ShortcutManager.cpp
 * @brief Cutter's default shortcut table and how shortcuts are matched.
 */

#include "IsolatedSettings.h"

#include "shortcuts/ShortcutManager.h"

#include <QtTest/QtTest>

class TestShortcutManager : public QObject
{
    Q_OBJECT

private slots:
    void defaultShortcutsDoNotConflictWithinContext();
    void everyShortcutHasKeyOnThisPlatform();
    void matchesEveryKeyOfShortcut();
};

/**
 * @brief No two default shortcuts share a key in the same context.
 *
 * Qt treats such a key as ambiguous, so neither shortcut fires.
 */
void TestShortcutManager::defaultShortcutsDoNotConflictWithinContext()
{
    const QHash<QString, Shortcut> shortcuts = Shortcuts()->getAllShortcuts();
    QHash<QString, QString> owners;
    QStringList conflicts;

    for (auto it = shortcuts.cbegin(); it != shortcuts.cend(); ++it) {
        for (const QKeySequence &sequence : it.value().keySequences) {
            if (sequence.isEmpty()) {
                continue;
            }
            const QString key = QString::fromLatin1(it.value().context) + '|'
                    + sequence.toString(QKeySequence::PortableText);
            const QString owner = owners.value(key);
            if (!owner.isEmpty() && owner != it.key()) {
                conflicts << QString("%1 and %2 share %3").arg(owner, it.key(), key);
            } else {
                owners.insert(key, it.key());
            }
        }
    }

    conflicts.sort();
    QVERIFY2(conflicts.isEmpty(), qPrintable(conflicts.join("; ")));
}

/**
 * @brief Every default shortcut has a key on this OS; platform key bindings can be empty.
 */
void TestShortcutManager::everyShortcutHasKeyOnThisPlatform()
{
    const QHash<QString, Shortcut> shortcuts = Shortcuts()->getAllShortcuts();
    QStringList missing;

    for (auto it = shortcuts.cbegin(); it != shortcuts.cend(); ++it) {
        bool hasKey = false;
        for (const QKeySequence &sequence : it.value().keySequences) {
            hasKey = hasKey || !sequence.isEmpty();
        }
        if (!hasKey) {
            missing << it.key();
        }
    }

    missing.sort();
    QVERIFY2(missing.isEmpty(), qPrintable("no key: " + missing.join(", ")));
}

/**
 * @brief matchesKeySequence accepts every key of a shortcut, not only the first.
 */
void TestShortcutManager::matchesEveryKeyOfShortcut()
{
    const QList<QKeySequence> zoomIn = Shortcuts()->getKeySequences("General.zoomIn");
    QVERIFY(zoomIn.size() > 1);

    for (const QKeySequence &sequence : zoomIn) {
        QVERIFY(Shortcuts()->matchesKeySequence("General.zoomIn", sequence));
        QVERIFY(!Shortcuts()->matchesKeySequence("General.zoomOut", sequence));
    }
    QVERIFY(!Shortcuts()->matchesKeySequence("General.zoomIn", QKeySequence()));
}

CUTTER_UNIT_TEST_MAIN(TestShortcutManager)
#include "test_ShortcutManager.moc"
