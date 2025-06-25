#ifndef SHORTCUTMANAGER_H
#define SHORTCUTMANAGER_H

#include <QObject>
#include <QWidget>
#include <QKeySequence>
#include "DefaultShortcuts.h"

class ShortcutManager : public QObject
{
    Q_OBJECT

public:
    static ShortcutManager *getInstance();

    Shortcut getShortcut(const QString &name);
    QKeySequence getKeySequence(const QString &name);
    QList<QKeySequence> getKeySequences(const QString &name);
    QString getText(const QString &name);
    QHash<QString, Shortcut> getAllShortcuts();

    ShortcutManager();

private:
    static ShortcutManager *instance;
};

#define Shortcuts() (ShortcutManager::getInstance())

#endif // SHORTCUTMANAGER_H
