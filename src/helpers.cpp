#include "helpers.h"

QHelpers::QHelpers(QObject *parent) :
    QObject(parent)
{
    // Meow
}

void QHelpers::normalizeFont(QPlainTextEdit *edit) {
    #ifdef Q_OS_LINUX
        // Add custom monospaced font
        QFontDatabase fontDB;
        fontDB.addApplicationFont(":/new/prefix1/fonts/Inconsolata-Regular.ttf");

        QFont anonFont("Inconsolata", 12);
        QTextDocument *out_doc = edit->document();
        out_doc->setDefaultFont(anonFont);
    #endif
}

void QHelpers::normalizeEditFont(QTextEdit *edit) {
    #ifdef Q_OS_LINUX
        // Add custom monospaced font
        QFontDatabase fontDB;
        fontDB.addApplicationFont(":/new/prefix1/fonts/Inconsolata-Regular.ttf");

        QFont anonFont("Inconsolata", 12);
        QTextDocument *out_doc = edit->document();
        out_doc->setDefaultFont(anonFont);
    #endif
}
