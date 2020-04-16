#include "ColorPicker.h"
#include "ui_ColorPicker.h"

#include <QPaintEvent>
#include <QPainter>
#include <QMouseEvent>
#include <QDesktopWidget>
#include <QPixmap>
#include <QCursor>
#include <QScreen>

using namespace ColorPickerHelpers;

ColorPickArea::ColorPickArea(QWidget *parent) : ColorPickerWidget(parent)
{
    setMouseTracking(false);
}

void ColorPickArea::paintEvent(QPaintEvent* event)
{
    QPainter p(this);

    for (int x = event->rect().x(); x <= event->rect().right(); x++) {
        for (int y = event->rect().y(); y <= event->rect().bottom(); y++) {
            qreal h, s, v;
            QColor c = pointToColor(x, y);
            c.getHsvF(&h, &s, &v);
            c.setHsvF(h, s, 1);
            p.setPen(c);
            p.drawPoint(x, y);
        }
    }

    p.setPen(QPen(Qt::black, 3));
    QPoint curr = colorToPoint(currColor);
    p.drawLine(curr - QPoint(0, 8), curr + QPoint(0, 8));
    p.drawLine(curr - QPoint(8, 0), curr + QPoint(8, 0));

    p.end();
}

void ColorPickArea::setColor(const QColor& c)
{
    if (c == currColor) {
        return;
    }
    QPoint p1 = colorToPoint(currColor);
    QPoint p2 = colorToPoint(c);
    currColor = c;
    repaint(QRect(p2 - QPoint(10, 10), p2 + QPoint(10, 10)));
    repaint(QRect(p1 - QPoint(10, 10), p1 + QPoint(10, 10)));

    emit colorChanged(currColor);
}

ColorPickerWidget::ColorPickerWidget(QWidget* parent) : ColorPickWidgetAbstract(parent)
{

}

void ColorPickerWidget::mouseReleaseEvent(QMouseEvent* event)
{
    mouseEvent(event);
}

void ColorPickerWidget::mousePressEvent(QMouseEvent* event)
{
    mouseEvent(event);
}

void ColorPickerWidget::mouseMoveEvent(QMouseEvent* event)
{
    mouseEvent(event);
}

QColor ColorPickArea::pointToColor(int x, int y) const
{
    QColor color;
    qreal h, s, v, a;
    currColor.getHsvF(&h, &s, &v, &a);
    color.setHsvF(qreal(x) / width(),
                  1.0 - qreal(y) / height(),
                  v, a);
    return color;
}

QPoint ColorPickArea::colorToPoint(const QColor& color) const
{
    qreal h, s, v;
    color.getHsvF(&h, &s, &v);
    return QPointF(h * width(), (1.0 - s) * height()).toPoint();
}

void ColorPickerWidget::mouseEvent(QMouseEvent* event)
{
    QPoint pos = event->pos();
    if (!rect().contains(pos.x(), rect().y())) {
        pos.setX(rect().x() < pos.x()
                 ? rect().right() + 1
                 : rect().x());
    }
    if (!rect().contains(rect().x(), pos.y())) {
        pos.setY(rect().y() < pos.y()
                 ? rect().bottom() + 1
                 : rect().y());
    }
    setColor(pointToColor(pos.x(), pos.y()));
}

void ColorValueBar::setColor(const QColor& c)
{
    if (c == currColor) {
        return;
    }
    currColor = c;
    repaint();

    emit colorChanged(currColor);
}

void ColorValueBar::paintEvent(QPaintEvent* event)
{
    QPainter p(this);
    QColor color = currColor;
    qreal h, s, v;
    currColor.getHsvF(&h, &s, &v);
    v = 1.0 - v;

    const int triangleSize = 10;
    QRect barRect = rect();
    barRect.setWidth(barRect.width() - triangleSize);


    for (int y = barRect.y(); y <= barRect.bottom(); y++) {
        color.setHsvF(h, s, 1.0 - qreal(y) / height());
        p.setPen(color);
        p.drawLine(barRect.x(), y, barRect.right(), y);
    }

    QRectF triangleRect = QRectF(barRect.right(), v * height() - triangleSize / 2,
                                 triangleSize, triangleSize);

    QPainterPath path;
    path.moveTo(triangleRect.left(), triangleRect.top() + triangleRect.height() / 2);
    path.lineTo(triangleRect.topRight());
    path.lineTo(triangleRect.bottomRight());
    path.lineTo(triangleRect.left(), triangleRect.top() + triangleRect.height() / 2);

    p.fillPath(path, palette().text().color());

    p.end();
    QWidget::paintEvent(event);
}

QColor ColorValueBar::pointToColor(int x, int y) const
{
    Q_UNUSED(x)
    QColor color = currColor;
    qreal h, s, v, a;
    color.getHsvF(&h, &s, &v, &a);
    color.setHsvF(h, s, 1.0 - qreal(y) / height(), a);
    return color;
}

QPoint ColorValueBar::colorToPoint(const QColor& color) const
{
    qreal h, s, v;
    color.getHsvF(&h, &s, &v);
    return QPoint(rect().x(), int((1.0 - v) * height()));
}

ColorPicker::ColorPicker(QWidget* parent) :
    ColorPickWidgetAbstract(parent),
    ui(new Ui::ColorPicker),
    pickingFromScreen(false)
{
    ui->setupUi(this);
    connect(ui->colorPickArea, &ColorPickArea::colorChanged,
            this, &ColorPicker::setColor);
    connect(ui->valuePickBar, &ColorValueBar::colorChanged,
            this, &ColorPicker::setColor);
    connect(ui->alphaChannelBar, &AlphaChannelBar::colorChanged,
            this, [this](const QColor& color) { emit colorChanged(color); });
    connect(this, &ColorPicker::colorChanged,
            ui->colorPickArea, &ColorPickArea::setColor);
    connect(this, &ColorPicker::colorChanged,
            ui->valuePickBar, &ColorValueBar::setColor);
    connect(this, &ColorPicker::colorChanged,
            ui->colorShow, &ColorShowWidget::setColor);
    connect(this, &ColorPicker::colorChanged,
            ui->alphaChannelBar, &AlphaChannelBar::setColor);

    connect(ui->hueSpinBox, static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged),
            this, &ColorPicker::colorChannelChanged);
    connect(ui->satSpinBox, static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged),
            this, &ColorPicker::colorChannelChanged);
    connect(ui->valSpinBox, static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged),
            this, &ColorPicker::colorChannelChanged);
    connect(ui->redSpinBox, static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged),
            this, &ColorPicker::colorChannelChanged);
    connect(ui->blueSpinBox, static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged),
            this, &ColorPicker::colorChannelChanged);
    connect(ui->greenSpinBox, static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged),
            this, &ColorPicker::colorChannelChanged);

    connect(ui->hexLineEdit, &QLineEdit::textChanged, this, &ColorPicker::colorChannelChanged);

    connect(ui->pickColorFromScreenButton, &QPushButton::clicked, this, &ColorPicker::startPickingFromScreen);
}

ColorPicker::~ColorPicker()
{
    if (pickingFromScreen) {
        setColor(getColorAtMouse());
        stopPickingFromScreen();
    }
}

void ColorPicker::setColor(const QColor& color)
{
    updateColor(color);
    emit colorChanged(currColor);
}

void ColorPicker::colorChannelChanged()
{
    QString txt = ui->hexLineEdit->text();
    // Regex pattern below mimics the behaviour of former RegExp::exactMatch()
    if (!QRegularExpression("\\A(?:#[0-9a-fA-F]{6})\\z").match(txt).hasMatch()) {
        return;
    }
    QColor hexColor = txt;

    int h, s, v;
    h = ui->hueSpinBox->value();
    s = ui->satSpinBox->value();
    v = ui->valSpinBox->value();
    QColor hsvColor;
    hsvColor.setHsv(h, s, v);

    int r, g, b;
    r = ui->redSpinBox->value();
    g = ui->greenSpinBox->value();
    b = ui->blueSpinBox->value();
    QColor rgbColor;
    rgbColor.setRgb(r, g, b);

    if (hexColor.isValid() && hexColor != currColor) {
        setColor(hexColor);
    } else if (rgbColor.isValid() && rgbColor != currColor) {
        setColor(rgbColor);
    } else if (hsvColor.isValid() && hsvColor != currColor) {
        setColor(hsvColor);
    }
}

void ColorPicker::updateColor(const QColor& color)
{
    QSignalBlocker s0(ui->redSpinBox);
    QSignalBlocker s1(ui->blueSpinBox);
    QSignalBlocker s2(ui->greenSpinBox);

    QSignalBlocker s3(ui->valSpinBox);
    QSignalBlocker s4(ui->satSpinBox);
    QSignalBlocker s5(ui->hueSpinBox);

    QSignalBlocker s6(ui->hexLineEdit);

    QSignalBlocker s7(ui->alphaChannelBar);
    QSignalBlocker s8(ui->colorPickArea);
    QSignalBlocker s9(ui->colorShow);
    QSignalBlocker s10(ui->valuePickBar);

    currColor = color;

    ui->hexLineEdit->setText(currColor.name());

    int h, s, v;
    currColor.getHsv(&h, &s, &v);
    ui->hueSpinBox->setValue(h);
    ui->satSpinBox->setValue(s);
    ui->valSpinBox->setValue(v);

    int r, g, b;
    currColor.getRgb(&r, &g, &b);
    ui->redSpinBox->setValue(r);
    ui->greenSpinBox->setValue(g);
    ui->blueSpinBox->setValue(b);

    ui->valuePickBar->setColor(color);
    ui->colorPickArea->setColor(color);
    ui->colorShow->setColor(color);
    ui->alphaChannelBar->setColor(color);
}

void ColorPicker::startPickingFromScreen()
{
    if (!pickingFromScreen) {
        setMouseTracking(true);
        grabMouse(Qt::CursorShape::CrossCursor);
        pickingFromScreen = true;
        bufferColor = currColor;
    }
}

void ColorPicker::mouseReleaseEvent(QMouseEvent* event)
{
    if (pickingFromScreen) {
        setColor(getColorAtMouse());
        pickingFromScreen = false;
        setMouseTracking(false);
        releaseMouse();
    }
    QWidget::mouseReleaseEvent(event);
}

void ColorPicker::mouseMoveEvent(QMouseEvent* event)
{
    if (pickingFromScreen) {
        updateColor(getColorAtMouse());
    }
    QWidget::mouseMoveEvent(event);
}

QColor ColorPicker::getColorAtMouse()
{
    const QDesktopWidget *desktop = QApplication::desktop();
    const QPixmap pixmap = QGuiApplication::screens().at(desktop->screenNumber())
                           ->grabWindow(desktop->winId(),
                                        QCursor::pos().x(), QCursor::pos().y(), 1, 1);
    return QColor(pixmap.toImage().pixel(0, 0));
}

bool ColorPicker::isPickingFromScreen() const
{
    return pickingFromScreen;
}

void ColorPicker::setAlphaEnabled(bool enabled)
{
    ui->alphaChannelBar->setVisible(enabled);
}

void ColorPicker::stopPickingFromScreen()
{
    if (pickingFromScreen) {
        pickingFromScreen = false;
        updateColor(bufferColor);
        releaseMouse();
        setMouseTracking(false);
    }
}

ColorShowWidget::ColorShowWidget(QWidget* parent) : ColorPickWidgetAbstract(parent) { }

void ColorShowWidget::setColor(const QColor& c)
{
    currColor = c;
    repaint();
}

void ColorShowWidget::paintEvent(QPaintEvent* event)
{
    QPainter p(this);
    const int miniRectWidth = rect().width() / 2;
    for (int y = rect().topLeft().ry(); y < rect().bottomRight().ry(); y++) {
        for (int x = rect().topLeft().rx(); x < rect().bottomRight().rx(); x++) {
            p.setPen(((x % miniRectWidth) / (miniRectWidth / 2)) == ((y % miniRectWidth) / (miniRectWidth / 2))
                     ? Qt::white
                     : Qt::black);
            p.drawPoint(x, y);
        }
    }
    p.setPen(currColor);
    p.setBrush(QBrush(currColor));
    p.drawRect(event->rect());
    p.end();
}

void AlphaChannelBar::setColor(const QColor& c)
{
    if (c == currColor) {
        return;
    }
    currColor = c;
    repaint();
    emit colorChanged(currColor);
}

void AlphaChannelBar::paintEvent(QPaintEvent* event)
{
    QPainter p(this);
    QRect barRect = rect();

    qreal h, s, v, a;
    currColor.getHsvF(&h, &s, &v, &a);
    a = 1.0 - a;
    const int triangleSize = 10;

    barRect.setWidth(barRect.width() - triangleSize);

    const int miniRectWidth = barRect.width() / 2;
    for (int y = barRect.topLeft().ry(); y < barRect.bottomRight().ry(); y++) {
        for (int x = barRect.topLeft().rx(); x < barRect.bottomRight().rx(); x++) {
            p.setPen(((x % miniRectWidth) / (miniRectWidth / 2)) == ((y % miniRectWidth) / (miniRectWidth / 2))
                     ? Qt::white
                     : Qt::black);
            p.drawPoint(x, y);
            p.setPen(pointToColor(x, y));
            p.drawPoint(x, y);
        }
    }

    QRectF triangleRect = QRectF(barRect.right(), a * height() - triangleSize / 2,
                                 triangleSize, triangleSize);

    QPainterPath path;
    path.moveTo(triangleRect.left(), triangleRect.top() + triangleRect.height() / 2);
    path.lineTo(triangleRect.topRight());
    path.lineTo(triangleRect.bottomRight());
    path.lineTo(triangleRect.left(), triangleRect.top() + triangleRect.height() / 2);
    p.fillPath(path, palette().text().color());

    p.end();
    QWidget::paintEvent(event);
}

QColor AlphaChannelBar::pointToColor(int x, int y) const
{
    Q_UNUSED(x)
    QColor color = currColor;
    qreal h, s, v;
    color.getHsvF(&h, &s, &v);
    color.setHsvF(h, s, v, 1.0 - qreal(y) / height());
    return color;
}

QPoint AlphaChannelBar::colorToPoint(const QColor &) const
{
    return QPoint();
}
