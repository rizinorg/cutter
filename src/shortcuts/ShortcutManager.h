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

    Shortcut getShortcut(const QString &id);
    QKeySequence getKeySequence(const QString &id);
    QList<QKeySequence> getKeySequences(const QString &id);
    const char *getContext(const QString &id);
    const char *getText(const QString &id);
    QHash<QString, Shortcut> getAllShortcuts();

    QAction *makeAction(const QString &id, QWidget *parent);
    void setupAction(QAction &action, const QString &id);

    ShortcutManager();

private:
    static ShortcutManager *instance;
};

#define Shortcuts() (ShortcutManager::getInstance())

#endif // SHORTCUTMANAGER_H
