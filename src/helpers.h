#ifndef QHELPERS_H
#define QHELPERS_H

class QPlainTextEdit;
class QTextEdit;
class QString;

namespace qhelpers
{
    void normalizeFont(QPlainTextEdit *edit);
    void normalizeEditFont(QTextEdit *edit);

    QString uniqueProjectName(const QString &filename);
}

#endif // HELPERS_H
