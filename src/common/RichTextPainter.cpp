/* x64dbg RichTextPainter */
#include "RichTextPainter.h"

#include "CachedFontMetrics.h"
#include "common/Configuration.h"

#include <QPainter>
#include <QTextBlock>
#include <QTextFragment>

// TODO: fix performance (possibly use QTextLayout?)

template<typename T>
void RichTextPainter::paintRichText(QPainter *painter, T x, T y, T w, T h, T xinc,
                                    const List &richText, CachedFontMetrics<T> *fontMetrics)
{
    QPen pen;
    QPen highlightPen;
    QBrush brush(Qt::cyan);

    T fontAscent = fontMetrics->ascent();
    T baselineY = y + fontAscent;

    for (const CustomRichText &curRichText : richText) {
        T textWidth = fontMetrics->width(curRichText.text);
        T backgroundWidth = textWidth;
        if (backgroundWidth + xinc > w) {
            backgroundWidth = w - xinc;
        }
        if (backgroundWidth <= 0) { // stop drawing when going outside the specified width
            break;
        }
        switch (curRichText.flags) {
        case FlagNone: // defaults
            pen.setColor(ConfigColor("btext").name());
            painter->setPen(pen);
            break;
        case FlagColor: // color only
            pen.setColor(curRichText.textColor);
            painter->setPen(pen);
            break;
        case FlagBackground: // background only
            if (backgroundWidth > 0 && curRichText.textBackground.alpha()) {
                brush.setColor(curRichText.textBackground);
                painter->fillRect(QRectF(x + xinc, y, backgroundWidth, h), brush);
            }
            break;
        case FlagAll: // color+background
            if (backgroundWidth > 0 && curRichText.textBackground.alpha()) {
                brush.setColor(curRichText.textBackground);
                painter->fillRect(QRectF(x + xinc, y, backgroundWidth, h), brush);
            }
            pen.setColor(curRichText.textColor);
            painter->setPen(pen);
            break;
        }

        painter->drawText(QPointF(x + xinc, baselineY), curRichText.text);

        if (curRichText.highlight && curRichText.highlightColor.alpha()) {
            highlightPen.setColor(curRichText.highlightColor);
            highlightPen.setWidth(curRichText.highlightWidth);
            painter->setPen(highlightPen);
            T highlightOffsetX = curRichText.highlightConnectPrev ? -1 : 1;
            painter->drawLine(x + xinc + highlightOffsetX, y + h - 1,
                              x + xinc + backgroundWidth - 1, y + h - 1);
        }
        xinc += textWidth;
    }
}

template void RichTextPainter::paintRichText<qreal>(QPainter *painter, qreal x, qreal y, qreal w,
                                                    qreal h, qreal xinc, const List &richText,
                                                    CachedFontMetrics<qreal> *fontMetrics);

RichTextPainter::List RichTextPainter::fromTextDocument(const QTextDocument &doc)
{
    List r;

    for (QTextBlock block = doc.begin(); block != doc.end(); block = block.next()) {
        for (QTextBlock::iterator it = block.begin(); it != block.end(); ++it) {
            const QTextFragment fragment = it.fragment();
            const QTextCharFormat format = fragment.charFormat();

            CustomRichText text;
            text.text = fragment.text();
            text.textColor = format.foreground().color();
            text.textBackground = format.background().color();

            const bool hasForeground = format.hasProperty(QTextFormat::ForegroundBrush);
            const bool hasBackground = format.hasProperty(QTextFormat::BackgroundBrush);

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
        const int textLength = text.text.size();
        if (cols + textLength <= maxCols) {
            r.push_back(text);
            cols += textLength;
        } else if (cols == maxCols) {
            break;
        } else {
            CustomRichText croppedText = text;
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
                text.text.replace(text.text.length() - indicatorCropLength, indicatorCropLength,
                                  indicator);
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
