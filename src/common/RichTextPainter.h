/* x64dbg RichTextPainter */
#ifndef RICHTEXTPAINTER_H
#define RICHTEXTPAINTER_H

#include "common/Metrics.h"

#include <QString>
#include <QTextDocument>
#include <QColor>
#include <vector>

class QFontMetricsF;
template<typename T> class CachedFontMetrics;
class QPainter;

class RichTextPainter
{
public:
    //structures
    enum CustomRichTextFlags {
        FlagNone,
        FlagColor,
        FlagBackground,
        FlagAll
    };

    struct CustomRichText_t {
        QString text;
        QColor textColor;
        QColor textBackground;
        CustomRichTextFlags flags;
        bool highlight = false;
        QColor highlightColor;
        int highlightWidth = 2;
        bool highlightConnectPrev = false;
    };

    typedef std::vector<CustomRichText_t> List;

    //functions
    template<typename T = qreal>
    static void paintRichText(QPainter *painter, T x, T y, T w, T h, T xinc,
                              const List &richText, CachedFontMetrics<T> *fontMetrics);
    static void htmlRichText(const List &richText, QString &textHtml, QString &textPlain);

    static List fromTextDocument(const QTextDocument &doc);

    static List cropped(const List &richText, int maxCols, const QString &indicator = nullptr,
                        bool *croppedOut = nullptr);
};

#endif // RICHTEXTPAINTER_H
