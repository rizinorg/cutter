#include "helpers.h"

#include <QPlainTextEdit>
#include <QTextEdit>

namespace qhelpers
{

// TODO: wouldn't it be enough to setFont on the QWidget?

void normalizeFont(QPlainTextEdit *edit) {
    #ifdef Q_OS_LINUX
        QFont anonFont("Inconsolata", 12);
        QTextDocument *out_doc = edit->document();
        out_doc->setDefaultFont(anonFont);
    #endif
}

void normalizeEditFont(QTextEdit *edit) {
    #ifdef Q_OS_LINUX
        QFont anonFont("Inconsolata", 12);
        QTextDocument *out_doc = edit->document();
        out_doc->setDefaultFont(anonFont);
    #endif
}

} // end namespace
