#include "ShortcutManager.h"
#include <QCoreApplication>
#include <QDebug>

Q_GLOBAL_STATIC(ShortcutManager, uniqueInstance)

ShortcutManager *ShortcutManager::getInstance()
{
    return uniqueInstance;
}

ShortcutManager::ShortcutManager() {}

QList<QKeySequence> ShortcutManager::getKeySequences(const QString &id)
{
    const auto &defaultShortcuts = getDefaultShortcuts();

    if (!defaultShortcuts.contains(id)) {
        qWarning() << "Can't find shortcut for" << id;
        return {};
    }

    QList<QKeySequence> ksq = getCustomKeySequences(id);
    if (ksq.isEmpty()) { // No custom keySequence set, return default
        ksq = defaultShortcuts.value(id).keySequences;
    }
    return ksq;
}

QKeySequence ShortcutManager::getKeySequence(const QString &id)
{
    const QList<QKeySequence> sequences = getKeySequences(id);
    return sequences.isEmpty() ? QKeySequence() : sequences.first();
}

Shortcut ShortcutManager::getShortcut(const QString &id)
{
    const auto &defaultShortcuts = getDefaultShortcuts();
    if (!defaultShortcuts.contains(id)) {
        qWarning() << "Can't find shortcut for" << id;
        return {};
    }

    Shortcut result = defaultShortcuts.value(id);

    QList<QKeySequence> customKeySequences = getKeySequences(id);
    if (!customKeySequences.isEmpty()) {
        result.keySequences = customKeySequences;
    }
    return result;
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

QAction *ShortcutManager::makeAction(const QString &id, QObject *parent)
{

    QAction *action = new QAction(parent);
    setupAction(*action, id);
    return action;
}

void ShortcutManager::setupAction(QAction &action, const QString &id)
{
    Shortcut s = getShortcut(id);
    action.setShortcuts(s.keySequences);
    action.setText(QCoreApplication::translate(s.context, s.text));
}

QShortcut *ShortcutManager::makeQShortcut(const QString &id, QWidget *parent)
{
    QShortcut *shortcut = new QShortcut(parent);
    QList<QKeySequence> keySequences = getKeySequences(id);
    if (!keySequences.isEmpty()) {
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
        shortcut->setKeys(keySequences);
#else
        shortcut->setKey(keySequences.first());
#endif
    }
    return shortcut;
}

bool ShortcutManager::matchesKeySequence(const QString &id, const QKeySequence &keySeq)
{
    const auto sequences = getKeySequences(id);
    for (const QKeySequence &seq : sequences) {
        if (seq == keySeq) {
            return true;
        }
    }
    return false;
}

QList<QKeySequence> ShortcutManager::getCustomKeySequences(const QString &id)
{
    return {}; // Custom shortcut support is not implemented yet
}
