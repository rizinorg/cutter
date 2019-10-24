#ifndef CUTTER_SELECTIONHIGHLIGHT_H
#define CUTTER_SELECTIONHIGHLIGHT_H

#include <QTextEdit>

class QPlainTextEdit;
class QString;

QList<QTextEdit::ExtraSelection> createSameWordsSelections(QPlainTextEdit *textEdit, const QString &word);
QTextEdit::ExtraSelection createLineHighlightSelection(const QTextCursor &cursor);

#endif //CUTTER_SELECTIONHIGHLIGHT_H
