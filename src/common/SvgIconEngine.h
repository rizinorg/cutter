
#ifndef SVGICONENGINE_H
#define SVGICONENGINE_H

#include <QIconEngine>

class SvgIconEngine: public QIconEngine
{
private:
    QByteArray svgData;

public:
    explicit SvgIconEngine(const QString &filename);
    explicit SvgIconEngine(const QByteArray &svgData);

    SvgIconEngine(const QString &filename, QColor tintColor);
    SvgIconEngine(const QByteArray &svgData, QColor tintColor);

    void paint(QPainter *painter, const QRect &rect, QIcon::Mode mode, QIcon::State state) override;
    QIconEngine *clone() const override;
    QPixmap pixmap(const QSize &size, QIcon::Mode mode, QIcon::State state) override;

};

#endif //SVGICONENGINE_H
