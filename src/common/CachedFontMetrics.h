#ifndef CACHEDFONTMETRICS_H
#define CACHEDFONTMETRICS_H

#include "common/Metrics.h"

#include <QObject>
#include <QFont>
#include <QFontMetrics>

template<typename T>
class CachedFontMetrics
{
public:
    explicit CachedFontMetrics(const QFont &font)
        : mFontMetrics(font)
    {
        memset(mWidths, 0, sizeof(mWidths));
        mHeight = mFontMetrics.height();
    }

    T width(const QChar &ch)
    {
        //return mFontMetrics.width(ch);
        auto unicode = ch.unicode();
        if (unicode >= 0xD800) {
            if (unicode >= 0xE000)
                unicode -= 0xE000 - 0xD800;
            else
                // is lonely surrogate
                return fetchWidth(ch);
        }
        if (!mWidths[unicode])
            return mWidths[unicode] = fetchWidth(ch);
        return mWidths[unicode];
    }

    T width(const QString &text)
    {
        T result = 0;
        QChar temp;
        for (const QChar &ch : text) {
            if (ch.isHighSurrogate())
                temp = ch;
            else if (ch.isLowSurrogate())
                result += fetchWidth(QString(temp) + ch);
            else
                result += width(ch);
        }
        return result;
    }

    T height()
    {
        return mHeight;
    }

    T position(const QString &text, T offset)
    {
        T curWidth = 0;
        QChar temp;

        for (int i = 0; i < text.length(); i++) {
            QChar ch = text[i];

            if (ch.isHighSurrogate())
                temp = ch;
            else if (ch.isLowSurrogate())
                curWidth += fetchWidth(QString(temp) + ch);
            else
                curWidth += width(ch);

            if (curWidth >= offset) {
                return i;
            }
        }

        return -1;
    }

private:
    typename Metrics<T>::FontMetrics mFontMetrics;
    T mWidths[0x10000 - 0xE000 + 0xD800];
    T mHeight;

    T fetchWidth(QChar c)
    {
#if QT_VERSION < QT_VERSION_CHECK(5,11,0)
        return mFontMetrics.width(c);
#else
        return mFontMetrics.horizontalAdvance(c);
#endif
    }

    T fetchWidth(const QString &s)
    {
#if QT_VERSION < QT_VERSION_CHECK(5,11,0)
        return mFontMetrics.width(s);
#else
        return mFontMetrics.horizontalAdvance(s);
#endif
    }
};

#endif // CACHEDFONTMETRICS_H
