#include "ShortcutManager.h"
#include "Configuration.h"

Q_GLOBAL_STATIC(ShortcutManager, uniqueInstance)

ShortcutManager *ShortcutManager::getInstance()
{
    return uniqueInstance;
}

ShortcutManager::ShortcutManager() {}

QList<QKeySequence> ShortcutManager::getKeySequences(const QString &name)
{
    const auto &defaultShortcuts = getDefaultShortcuts();
    QList<QKeySequence> ksq = Config()->getKeySequences(name);
    if (ksq.isEmpty()) { // No custom keySequence set, return default
        ksq = defaultShortcuts.value(name).keySequences;
    }
    return ksq;
}

QKeySequence ShortcutManager::getKeySequence(const QString &name)
{
    const QList<QKeySequence> sequences = getKeySequences(name);
    return sequences.isEmpty() ? QKeySequence() : sequences.first();
}

QString ShortcutManager::getText(const QString &name)
{
    const auto &defaultShortcuts = getDefaultShortcuts();
    return tr(qPrintable(defaultShortcuts.value(name).text));
}

Shortcut ShortcutManager::getShortcut(const QString &name)
{
    QList<QKeySequence> ksq = getKeySequences(name);
    QString text = getText(name);
    return { ksq, text };
}

QHash<QString, Shortcut> ShortcutManager::getAllShortcuts()
{
    const auto &defaultShortcuts = getDefaultShortcuts();
    QHash<QString, Shortcut> shortcuts;
    shortcuts.reserve(defaultShortcuts.size());

    for (auto it = defaultShortcuts.cbegin(); it != defaultShortcuts.cend(); ++it) {
        const QString name = it.key();
        Shortcut s = getShortcut(name);
        shortcuts.insert(name, s);
    }
    return shortcuts;
}
