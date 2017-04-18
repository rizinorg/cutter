#include "helpers.h"

#include <QPlainTextEdit>
#include <QTextEdit>
#include <QFileInfo>
#include <QCryptographicHash>

namespace qhelpers
{

    // TODO: wouldn't it be enough to setFont on the QWidget?

    void normalizeFont(QPlainTextEdit *edit)
    {
#ifdef Q_OS_LINUX
        QFont anonFont("Inconsolata", 12);
        QTextDocument *out_doc = edit->document();
        out_doc->setDefaultFont(anonFont);
#endif
    }

    void normalizeEditFont(QTextEdit *edit)
    {
#ifdef Q_OS_LINUX
        QFont anonFont("Inconsolata", 12);
        QTextDocument *out_doc = edit->document();
        out_doc->setDefaultFont(anonFont);
#endif
    }

    QString uniqueProjectName(const QString &filename)
    {
        const QByteArray fullHash(QCryptographicHash::hash(filename.toUtf8(), QCryptographicHash::Sha1));
        return QFileInfo(filename).fileName() + "_" + fullHash.toHex().left(10);
    }

} // end namespace
