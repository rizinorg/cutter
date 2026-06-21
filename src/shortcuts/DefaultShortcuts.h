#ifndef DEFAULTSHORTCUTS_H
#define DEFAULTSHORTCUTS_H

#include <QHash>
#include <QKeySequence>
#include <QPair>
#include <QString>

/**
 * @file DefaultShortcuts.h
 * Contains all of the default shortcuts for Cutter
 * Visually shown in @ref ShortcutOptionsWidget
 */

struct Shortcut
{
    QList<QKeySequence> keySequences;
    const char *text; // untranslated description
    const char *context;
};

const QHash<QString, Shortcut> &getDefaultShortcuts();

#endif // DEFAULTSHORTCUTS_H
