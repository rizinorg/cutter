
#ifndef SVGICONENGINE_H
#define SVGICONENGINE_H

#include <QIconEngine>
#include <QPalette>

class SvgIconEngine : public QIconEngine
{
private:
    QByteArray svgData;

public:
    explicit SvgIconEngine(const QString &filename);

    SvgIconEngine(const QString &filename, const QColor &tintColor);
    SvgIconEngine(const QString &filename, QPalette::ColorRole colorRole);

    void paint(QPainter *painter, const QRect &rect, QIcon::Mode mode, QIcon::State state) override;
    QIconEngine *clone() const override;
    QPixmap pixmap(const QSize &size, QIcon::Mode mode, QIcon::State state) override;
};

#endif // SVGICONENGINE_H
