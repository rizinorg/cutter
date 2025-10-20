#ifndef SHORTCUTMANAGER_H
#define SHORTCUTMANAGER_H

#include <QObject>
#include <QWidget>
#include <QKeySequence>
#include <QAction>
#include <QShortcut>
#include "DefaultShortcuts.h"

class ShortcutManager : public QObject
{
    Q_OBJECT

public:
    static ShortcutManager *getInstance();

    Shortcut getShortcut(const QString &id);
    QKeySequence getKeySequence(const QString &id);
    QList<QKeySequence> getKeySequences(const QString &id);
    QHash<QString, Shortcut> getAllShortcuts();

    QAction *makeAction(const QString &id, QObject *parent);
    void setupAction(QAction &action, const QString &id);

    QShortcut *makeQShortcut(const QString &id, QWidget *parent);

    /**
     * @brief Returns whether the given key sequence matches any key sequence assigned to the
     * specified shortcut ID.
     */
    bool matchesKeySequence(const QString &id, const QKeySequence &keySeq);

    /**
     * @brief placeholder for getting custom shortcuts set by the user.
     */
    QList<QKeySequence> getCustomKeySequences(const QString &id);

    ShortcutManager();

private:
    static ShortcutManager *instance;
};

#define Shortcuts() (ShortcutManager::getInstance())

#endif // SHORTCUTMANAGER_H
