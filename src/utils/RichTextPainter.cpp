/* x64dbg RichTextPainter */
#include "RichTextPainter.h"
#include "CachedFontMetrics.h"
#include <QPainter>
#include <QTextBlock>
#include <QTextFragment>

//TODO: fix performance (possibly use QTextLayout?)
void RichTextPainter::paintRichText(QPainter *painter, int x, int y, int w, int h, int xinc,
                                    const List &richText, CachedFontMetrics *fontMetrics)
{
    QPen pen;
    QPen highlightPen;
    QBrush brush(Qt::cyan);
    for (const CustomRichText_t &curRichText : richText) {
        int textWidth = fontMetrics->width(curRichText.text);
        int backgroundWidth = textWidth;
        if (backgroundWidth + xinc > w)
            backgroundWidth = w - xinc;
        if (backgroundWidth <= 0) //stop drawing when going outside the specified width
            break;
        switch (curRichText.flags) {
        case FlagNone: //defaults
            break;
        case FlagColor: //color only
            pen.setColor(curRichText.textColor);
            painter->setPen(pen);
            break;
        case FlagBackground: //background only
            if (backgroundWidth > 0 && curRichText.textBackground.alpha()) {
                brush.setColor(curRichText.textBackground);
                painter->fillRect(QRect(x + xinc, y, backgroundWidth, h), brush);
            }
            break;
        case FlagAll: //color+background
            if (backgroundWidth > 0 && curRichText.textBackground.alpha()) {
                brush.setColor(curRichText.textBackground);
                painter->fillRect(QRect(x + xinc, y, backgroundWidth, h), brush);
            }
            pen.setColor(curRichText.textColor);
            painter->setPen(pen);
            break;
        }
        painter->drawText(QRect(x + xinc, y, w - xinc, h), Qt::TextBypassShaping, curRichText.text);
        if (curRichText.highlight && curRichText.highlightColor.alpha()) {
            highlightPen.setColor(curRichText.highlightColor);
            highlightPen.setWidth(curRichText.highlightWidth);
            painter->setPen(highlightPen);
            int highlightOffsetX = curRichText.highlightConnectPrev ? -1 : 1;
            painter->drawLine(x + xinc + highlightOffsetX, y + h - 1, x + xinc + backgroundWidth - 1,
                              y + h - 1);
        }
        xinc += textWidth;
    }
}

/**
 * @brief RichTextPainter::htmlRichText Convert rich text in x64dbg to HTML, for use by other applications
 * @param richText The rich text to be converted to HTML format
 * @param textHtml The HTML source. Any previous content will be preserved and new content will be appended at the end.
 * @param textPlain The plain text. Any previous content will be preserved and new content will be appended at the end.
 */
void RichTextPainter::htmlRichText(const List &richText, QString &textHtml, QString &textPlain)
{
    for (const CustomRichText_t &curRichText : richText) {
        if (curRichText.text == " ") { //blank
            textHtml += " ";
            textPlain += " ";
            continue;
        }
        switch (curRichText.flags) {
        case FlagNone: //defaults
            textHtml += "<span>";
            break;
        case FlagColor: //color only
            textHtml += QString("<span style=\"color:%1\">").arg(curRichText.textColor.name());
            break;
        case FlagBackground: //background only
            if (curRichText.textBackground !=
                    Qt::transparent) // QColor::name() returns "#000000" for transparent color. That's not desired. Leave it blank.
                textHtml += QString("<span style=\"background-color:%1\">").arg(curRichText.textBackground.name());
            else
                textHtml += QString("<span>");
            break;
        case FlagAll: //color+background
            if (curRichText.textBackground !=
                    Qt::transparent) // QColor::name() returns "#000000" for transparent color. That's not desired. Leave it blank.
                textHtml += QString("<span style=\"color:%1; background-color:%2\">").arg(
                                curRichText.textColor.name(), curRichText.textBackground.name());
            else
                textHtml += QString("<span style=\"color:%1\">").arg(curRichText.textColor.name());
            break;
        }
        if (curRichText.highlight) //Underline highlighted token
            textHtml += "<u>";
        textHtml += curRichText.text.toHtmlEscaped();
        if (curRichText.highlight)
            textHtml += "</u>";
        textHtml += "</span>"; //Close the tag
        textPlain += curRichText.text;
    }
}

RichTextPainter::List RichTextPainter::fromTextDocument(const QTextDocument &doc)
{
    List r;

    for (QTextBlock block = doc.begin(); block != doc.end(); block = block.next()) {
        for (QTextBlock::iterator it = block.begin(); it != block.end(); it++) {
            QTextFragment fragment = it.fragment();
            QTextCharFormat format = fragment.charFormat();

            CustomRichText_t text;
            text.text = fragment.text();
            text.textColor = format.foreground().color();
            text.textBackground = format.background().color();

            bool hasForeground = format.hasProperty(QTextFormat::ForegroundBrush);
            bool hasBackground = format.hasProperty(QTextFormat::BackgroundBrush);

            if (hasForeground && !hasBackground) {
                text.flags = FlagColor;
            } else if (!hasForeground && hasBackground) {
                text.flags = FlagBackground;
            } else if (hasForeground && hasBackground) {
                text.flags = FlagAll;
            } else {
                text.flags = FlagNone;
            }

            r.push_back(text);
        }
    }

    return r;
}

RichTextPainter::List RichTextPainter::cropped(const RichTextPainter::List &richText, int maxCols,
                                               const QString &indicator, bool *croppedOut)
{
    List r;
    r.reserve(richText.size());

    int cols = 0;
    bool cropped = false;
    for (const auto &text : richText) {
        int textLength = text.text.size();
        if (cols + textLength <= maxCols) {
            r.push_back(text);
            cols += textLength;
        } else if (cols == maxCols) {
            break;
        } else {
            CustomRichText_t croppedText = text;
            croppedText.text.truncate(maxCols - cols);
            r.push_back(croppedText);
            cropped = true;
            break;
        }
    }

    if (cropped && !indicator.isEmpty()) {
        int indicatorCropLength = indicator.length();
        if (indicatorCropLength > maxCols) {
            indicatorCropLength = maxCols;
        }

        while (!r.empty()) {
            auto &text = r.back();

            if (text.text.length() >= indicatorCropLength) {
                text.text.replace(text.text.length() - indicatorCropLength, indicatorCropLength, indicator);
                break;
            }

            indicatorCropLength -= text.text.length();
            r.pop_back();
        }
    }

    if (croppedOut) {
        *croppedOut = cropped;
    }
    return r;
}
