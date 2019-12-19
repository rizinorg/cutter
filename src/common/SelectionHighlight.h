#ifndef CUTTER_SELECTIONHIGHLIGHT_H
#define CUTTER_SELECTIONHIGHLIGHT_H

#include <QTextEdit>

class QPlainTextEdit;
class QString;

/**
 * @brief createSameWordsSelections se
 * @param textEdit
 * @param word
 * @return
 */
QList<QTextEdit::ExtraSelection> createSameWordsSelections(QPlainTextEdit *textEdit, const QString &word);

/**
 * @brief createLineHighlight
 * @param cursor - a Cursor object represents the line to be highlighted
 * @param highlightColor - the color to be used for highlighting. The color is decided by the callee for different usages (BP, PC, Current line, ...)
 * @return ExtraSelection with highlighted line
 */
QTextEdit::ExtraSelection createLineHighlight(const QTextCursor &cursor, QColor highlightColor);

/**
 * @brief This function responsible to highlight the currently selected line 
 * @param cursor - a Cursor object represents the line to be highlighted
 * @return ExtraSelection with highlighted line
 */
QTextEdit::ExtraSelection createLineHighlightSelection(const QTextCursor &cursor);

/**
 * @brief This function responsible to highlight the program counter line 
 * @param cursor - a Cursor object represents the line to be highlighted
 * @return ExtraSelection with highlighted line
 */
QTextEdit::ExtraSelection createLineHighlightPC(const QTextCursor &cursor);

/**
 * @brief This function responsible to highlight a line with breakpoint
 * @param cursor - a Cursor object represents the line to be highlighted
 * @return ExtraSelection with highlighted line
 */
QTextEdit::ExtraSelection createLineHighlightBP(const QTextCursor &cursor);

#endif //CUTTER_SELECTIONHIGHLIGHT_H
