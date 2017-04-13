#ifndef QHELPERS_H
#define QHELPERS_H

#include <QString>

class QPlainTextEdit;
class QTextEdit;
class QString;
class QTreeWidget;

namespace qhelpers
{
    void normalizeFont(QPlainTextEdit *edit);
    void normalizeEditFont(QTextEdit *edit);

    QString uniqueProjectName(const QString &filename);

    void adjustColumns(QTreeWidget *tw);

    void appendRow(QTreeWidget *tw, const QString &str, const QString &str2 = QString(),
                   const QString &str3 = QString(), const QString &str4 = QString(), const QString &str5 = QString());

    void setVerticalScrollMode(QTreeWidget* tw);
}

#endif // HELPERS_H
