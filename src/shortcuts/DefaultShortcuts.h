#ifndef DEFAULTSHORTCUTS_H
#define DEFAULTSHORTCUTS_H

#include <QString>
#include <QKeySequence>
#include <QHash>
#include <QPair>

struct Shortcut
{
    QList<QKeySequence> keySequences;
    QString text;
};

const QHash<QString, Shortcut> &getDefaultShortcuts();

#endif // DEFAULTSHORTCUTS_H
