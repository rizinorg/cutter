#include "common/HighDpiPixmap.h"

#include <QGuiApplication>
#include <QScreen>

static qreal GetDevicePixelRatio(qreal devicePixelRatio)
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
    : QPixmap(int(width * GetDevicePixelRatio(devicePixelRatio)),
              int(height * GetDevicePixelRatio(devicePixelRatio)))
{
    setDevicePixelRatio(GetDevicePixelRatio(devicePixelRatio));
}
