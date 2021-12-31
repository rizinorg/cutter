#ifndef WIDGETSHORTCUTS_H
#define WIDGETSHORTCUTS_H

#include <QKeySequence>
#include <QHash>
#include <QString>

static const QHash<QString, QKeySequence> widgetShortcuts = {
    { "StringsWidget", Qt::SHIFT | Qt::Key_F12 },      { "GraphWidget", Qt::SHIFT | Qt::Key_G },
    { "ImportsWidget", Qt::SHIFT | Qt::Key_I },        { "ExportsWidget", Qt::SHIFT | Qt::Key_E },
    { "ConsoleWidget", Qt::CTRL | Qt::Key_QuoteLeft }, { "ConsoleWidgetAlternative", Qt::Key_Colon }
};

#endif
