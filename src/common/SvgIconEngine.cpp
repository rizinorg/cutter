
#include "SvgIconEngine.h"

#include <QSvgRenderer>
#include <QPainter>
#include <QFile>

#include "Helpers.h"

SvgIconEngine::SvgIconEngine(const QString &filename)
{
    QFile file(filename);
    file.open(QFile::ReadOnly);
    this->svgData = file.readAll();
}

SvgIconEngine::SvgIconEngine(const QByteArray &svgData)
{
    this->svgData = svgData;
}

SvgIconEngine::SvgIconEngine(const QString &filename, QColor tintColor) : SvgIconEngine(filename)
{
    this->svgData = qhelpers::applyColorToSvg(svgData, tintColor);
}

SvgIconEngine::SvgIconEngine(const QByteArray &svgData, QColor tintColor)
{
    this->svgData = qhelpers::applyColorToSvg(svgData, tintColor);
}

void SvgIconEngine::paint(QPainter *painter, const QRect &rect, QIcon::Mode /*mode*/,
                          QIcon::State /*state*/)
{
    QSvgRenderer renderer(svgData);
    renderer.render(painter, rect);
}

QIconEngine *SvgIconEngine::clone() const
{
    return new SvgIconEngine(*this);
}

QPixmap SvgIconEngine::pixmap(const QSize &size, QIcon::Mode mode, QIcon::State state)
{
    QImage img(size, QImage::Format_ARGB32);
    img.fill(qRgba(0, 0, 0, 0));
    QPixmap pix = QPixmap::fromImage(img, Qt::NoFormatConversion);
    {
        QPainter painter(&pix);
        QRect r(QPoint(0.0, 0.0), size);
        this->paint(&painter, r, mode, state);
    }
    return pix;
}
