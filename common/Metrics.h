
#ifndef METRICS_H
#define METRICS_H

#include <QtGlobal>

class QRect;
class QRectF;
class QFontMetrics;
class QFontMetricsF;

template<typename T> struct Metrics {};

template<> struct Metrics<int>
{
    using Rect = QRect;
    using FontMetrics = QFontMetrics;
};

template<> struct Metrics<qreal>
{
    using Rect = QRectF;
    using FontMetrics = QFontMetricsF;
};

#endif //METRICS_H
