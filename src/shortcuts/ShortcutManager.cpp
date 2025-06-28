#include "ShortcutManager.h"
#include "Configuration.h"
#include <QCoreApplication>

Q_GLOBAL_STATIC(ShortcutManager, uniqueInstance)

ShortcutManager *ShortcutManager::getInstance()
{
    return uniqueInstance;
}

ShortcutManager::ShortcutManager() {}

QList<QKeySequence> ShortcutManager::getKeySequences(const QString &id)
{
    const auto &defaultShortcuts = getDefaultShortcuts();
    QList<QKeySequence> ksq = Config()->getKeySequences(id);
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

const char *ShortcutManager::getText(const QString &id)
{
    const auto &defaultShortcuts = getDefaultShortcuts();
    return defaultShortcuts.value(id).text;
}

const char *ShortcutManager::getContext(const QString &id)
{
    const auto &defaultShortcuts = getDefaultShortcuts();
    return defaultShortcuts.value(id).context;
}

Shortcut ShortcutManager::getShortcut(const QString &id)
{
    Shortcut s;
    s.keySequences = getKeySequences(id);
    s.text = getText(id);
    s.context = getContext(id);
    return s;
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

QAction *ShortcutManager::makeAction(const QString &id, QWidget *parent)
{

    QAction *action = new QAction(parent);
    setupAction(*action, id);
    return action;
}

void ShortcutManager::setupAction(QAction &action, const QString &id)
{
    Shortcut shortcut = getShortcut(id);
    action.setShortcuts(shortcut.keySequences);
    action.setText(QCoreApplication::translate(shortcut.context, shortcut.text));
}
