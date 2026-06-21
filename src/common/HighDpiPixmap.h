#ifndef HIGHDPIPIXMAP_H
#define HIGHDPIPIXMAP_H

#include <QPixmap>

/**
 * @brief A QPixmap wrapper that automatically scales for high density displays
 */
class HighDpiPixmap : public QPixmap
{
public:
    HighDpiPixmap(int width, int height, qreal devicePixelRatio = -1);
};

#endif // HIGHDPIPIXMAP_H
