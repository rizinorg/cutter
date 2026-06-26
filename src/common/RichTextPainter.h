/* x64dbg RichTextPainter */
#ifndef RICHTEXTPAINTER_H
#define RICHTEXTPAINTER_H

#include "CutterDescriptions.h" // IWYU pragma: keep

#include <QColor>
#include <QString>
#include <QTextDocument>

#include <vector>

class QFontMetricsF;
template<typename T>
class CachedFontMetrics;
class QPainter;

/**
 * @namespace For efficient painting of text
 */
namespace RichTextPainter {

// structures
enum CustomRichTextFlags : ut8 { FlagNone, FlagColor, FlagBackground, FlagAll };

struct CustomRichText
{
    QString text;
    QColor textColor;
    QColor textBackground;
    CustomRichTextFlags flags;
    bool highlight = false;
    QColor highlightColor;
    int highlightWidth = 2;
    bool highlightConnectPrev = false;
};

typedef std::vector<CustomRichText> List;

// functions
template<typename T = qreal>
void paintRichText(QPainter *painter, T x, T y, T w, T h, T xinc, const List &richText,
                   CachedFontMetrics<T> *fontMetrics);

/**
 * @brief RichTextPainter::htmlRichText Convert rich text in x64dbg to HTML, for use by other
 * applications
 * @param richText The rich text to be converted to HTML format
 * @param[out] textHtml The HTML source. Any previous content will be preserved and new content will
 * be appended at the end.
 * @param[out] textPlain The plain text. Any previous content will be preserved and new content will
 * be appended at the end.
 */
void htmlRichText(const List &richText, QString &textHtml, QString &textPlain);

List fromTextDocument(const QTextDocument &doc);

/**
 * @brief Crops a list of rich text elements to a maximum column width.
 *
 * @param richText The input list of rich text segments.
 * @param maxCols The maximum character width allowed.
 * @param indicator String to append if text is cropped (e.g., "...").
 * @param[out] croppedOut Set to true if the text was actually truncated.
 * @return A new list containing the cropped segments.
 */
List cropped(const List &richText, int maxCols, const QString &indicator = nullptr,
             bool *croppedOut = nullptr);
}

#endif // RICHTEXTPAINTER_H
