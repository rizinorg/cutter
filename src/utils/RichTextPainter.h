/* x64dbg RichTextPainter */
#ifndef RICHTEXTPAINTER_H
#define RICHTEXTPAINTER_H

#include <QString>
#include <QTextDocument>
#include <QColor>
#include <vector>

class CachedFontMetrics;
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
    static void paintRichText(QPainter *painter, int x, int y, int w, int h, int xinc,
                              const List &richText, CachedFontMetrics *fontMetrics);
    static void htmlRichText(const List &richText, QString &textHtml, QString &textPlain);

    static List fromTextDocument(const QTextDocument &doc);

    static List cropped(const List &richText, int maxCols, const QString &indicator = nullptr,
                        bool *croppedOut = nullptr);
};

#endif // RICHTEXTPAINTER_H
