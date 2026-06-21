#include "common/HighDpiPixmap.h"

#include <QGuiApplication>
#include <QScreen>

static qreal getDevicePixelRatio(qreal devicePixelRatio)
{
    if (devicePixelRatio > 0) {
        return devicePixelRatio;
    }
    qreal ratio = 1;
    for (auto screen : QGuiApplication::screens()) {
        ratio = std::max(ratio, screen->devicePixelRatio());
    }
    return ratio;
}

HighDpiPixmap::HighDpiPixmap(int width, int height, qreal devicePixelRatio)
    : QPixmap(int(width * getDevicePixelRatio(devicePixelRatio)),
              int(height * getDevicePixelRatio(devicePixelRatio)))
{
    setDevicePixelRatio(getDevicePixelRatio(devicePixelRatio));
}
