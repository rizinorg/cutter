#ifndef CUTTER_SELECTIONHIGHLIGHT_H
#define CUTTER_SELECTIONHIGHLIGHT_H

#include <QTextEdit>

class QPlainTextEdit;
class QString;

/**
 * @brief Create selections for any occurences of the given word
 * @param textEdit TextEdit object to make selections in
 * @param word Word to make selections of
 * @return List of ExtraSelection with highlighted words
 */
QList<QTextEdit::ExtraSelection> createSameWordsSelections(QPlainTextEdit *textEdit,
                                                           const QString &word);

/**
 * @brief Highlight the currently selected line with given color
 * @param cursor A Cursor object represents the line to be highlighted
 * @param highlightColor The color to be used for highlighting. The color is decided by the callee
 * for different usages (BP, PC, Current line, ...)
 * @return ExtraSelection with highlighted line
 */
QTextEdit::ExtraSelection createLineHighlight(const QTextCursor &cursor, QColor highlightColor);

/**
 * @brief This function is responsible for highlighting the currently selected line
 *
 * Similar to @ref createLineHighlight() but uses "lineHighlght" value from config
 *
 * @param cursor A Cursor object represents the line to be highlighted
 * @return ExtraSelection with highlighted line
 */
QTextEdit::ExtraSelection createLineHighlightSelection(const QTextCursor &cursor);

/**
 * @brief This function is responsible for highlighting the program counter line
 * @param cursor A Cursor object represents the line to be highlighted
 * @return ExtraSelection with highlighted line
 */
QTextEdit::ExtraSelection createLineHighlightPC(const QTextCursor &cursor);

/**
 * @brief This function is responsible for highlighting a line that contains a breakpoint
 * @param cursor A Cursor object represents the line to be highlighted
 * @return ExtraSelection with highlighted line
 */
QTextEdit::ExtraSelection createLineHighlightBP(const QTextCursor &cursor);

#endif // CUTTER_SELECTIONHIGHLIGHT_H
