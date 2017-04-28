#ifndef QHELPERS_H
#define QHELPERS_H

#include <QString>

class QPlainTextEdit;
class QTextEdit;
class QString;
class QTreeWidget;
class QTreeWidgetItem;
class QAbstractItemView;

namespace qhelpers
{
    void normalizeFont(QPlainTextEdit *edit);
    void normalizeEditFont(QTextEdit *edit);

    QString uniqueProjectName(const QString &filename);

    void adjustColumns(QTreeWidget *tw, int columnCount = 0, int padding = 0);

    QTreeWidgetItem *appendRow(QTreeWidget *tw, const QString &str, const QString &str2 = QString(),
                               const QString &str3 = QString(), const QString &str4 = QString(), const QString &str5 = QString());

    void setVerticalScrollMode(QAbstractItemView *tw);
}

#endif // HELPERS_H
