#include "ShortcutManager.h"

#include <QCoreApplication>
#include <QDebug>
#include <qglobal.h>

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

Qt::KeyboardModifier ShortcutManager::convertKeyToModifer(const QKeySequence &sequence)
{
    if (sequence.isEmpty()) {
        return Qt::NoModifier;
    }
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    Qt::Key key = static_cast<Qt::Key>(sequence[0] & ~Qt::KeyboardModifierMask);
#else
    const QKeyCombination combo = sequence[0];
    const Qt::Key key = combo.key();
#endif
    switch (key) {
    case Qt::Key_Alt:
        return Qt::AltModifier;
    case Qt::Key_Shift:
        return Qt::ShiftModifier;

    case Qt::Key_Control:
        return Qt::ControlModifier;

    case Qt::Key_Meta:
        return Qt::MetaModifier;
    case Qt::Key_AltGr:
        return Qt::GroupSwitchModifier;

    default:
        return Qt::NoModifier;
    }
    return Qt::NoModifier;
}

Shortcut ShortcutManager::getShortcut(const QString &id)
{
    const auto &defaultShortcuts = getDefaultShortcuts();
    if (!defaultShortcuts.contains(id)) {
        qWarning() << "Can't find shortcut for" << id;
        return {};
    }

    Shortcut result = defaultShortcuts.value(id);

    const QList<QKeySequence> customKeySequences = getKeySequences(id);
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
        const QString &name = it.key();
        const Shortcut s = getShortcut(name);
        shortcuts.insert(name, s);
    }
    return shortcuts;
}

QAction *ShortcutManager::makeAction(const QString &id, QObject *parent)
{

    auto *action = new QAction(parent);
    setupAction(*action, id);
    return action;
}

void ShortcutManager::setupAction(QAction &action, const QString &id)
{
    const Shortcut s = getShortcut(id);
    action.setShortcuts(s.keySequences);
    action.setText(QCoreApplication::translate(s.context, s.text));
}

QShortcut *ShortcutManager::makeQShortcut(const QString &id, QWidget *parent)
{
    auto *shortcut = new QShortcut(parent);
    const QList<QKeySequence> keySequences = getKeySequences(id);
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

QList<QKeySequence> ShortcutManager::getCustomKeySequences(const QString & /*id*/)
{
    return {}; // Custom shortcut support is not implemented yet
}
