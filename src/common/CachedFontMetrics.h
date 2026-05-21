#ifndef CACHEDFONTMETRICS_H
#define CACHEDFONTMETRICS_H

#include "common/Metrics.h"

#include <QFont>
#include <QFontMetrics>
#include <QObject>

/**
 * @brief A cache class for font measurements
 *
 * Stores calculated widths in an array so when asking for a character's width
 * second time, it is returned instantly without any calculation
 *
 * @tparam T The type used for dimensions, such as int or qreal
 */
template<typename T>
class CachedFontMetrics
{
public:
    explicit CachedFontMetrics(const QFont &font)
        : fontMetrics(font), charWidths {}, fontHeight(fontMetrics.height())
    {
    }

    T width(const QChar &ch)
    {
        auto unicode = ch.unicode();
        if (unicode >= 0xD800) [[unlikely]] {
            if (unicode >= 0xE000) {
                unicode -= 0xE000 - 0xD800;
            } else {
                // is lonely surrogate
                return fetchWidth(ch);
            }
        }
        if (!charWidths[unicode]) [[unlikely]] {
            return charWidths[unicode] = fetchWidth(ch);
        }
        return charWidths[unicode];
    }

    T width(const QString &text)
    {
        T result = 0;
        QChar temp;
        for (const QChar &ch : text) {
            if (ch.isHighSurrogate()) [[unlikely]] {
                temp = ch;
            } else if (ch.isLowSurrogate()) [[unlikely]] {
                result += fetchWidth(QString(temp) + ch);
            } else [[likely]] {
                result += width(ch);
            }
        }
        return result;
    }

    T height() { return fontHeight; }

    T position(const QString &text, T offset)
    {
        T curWidth = 0;
        QChar temp;

        for (int i = 0; i < text.length(); i++) {
            const QChar ch = text[i];

            if (ch.isHighSurrogate()) [[unlikely]] {
                temp = ch;
            } else if (ch.isLowSurrogate()) [[unlikely]] {
                curWidth += fetchWidth(QString(temp) + ch);
            } else [[likely]] {
                curWidth += width(ch);
            }
            if (curWidth >= offset) {
                return i;
            }
        }

        return -1;
    }

    T ascent() { return fontMetrics.ascent(); }

private:
    typename Metrics<T>::FontMetrics fontMetrics;
    T charWidths[0x10000 - 0xE000 + 0xD800];
    T fontHeight;

    T fetchWidth(QChar c)
    {
#if QT_VERSION < QT_VERSION_CHECK(5, 11, 0)
        return fontMetrics.width(c);
#else
        return fontMetrics.horizontalAdvance(c);
#endif
    }

    T fetchWidth(const QString &s)
    {
#if QT_VERSION < QT_VERSION_CHECK(5, 11, 0)
        return fontMetrics.width(s);
#else
        return fontMetrics.horizontalAdvance(s);
#endif
    }
};

#endif // CACHEDFONTMETRICS_H
