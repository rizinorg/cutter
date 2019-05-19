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
    p.drawLine(curr - QPoint(0, 10), curr + QPoint(0, 10));
    p.drawLine(curr - QPoint(10, 0), curr + QPoint(10, 0));

    p.end();
    QWidget::paintEvent(event);
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
    qreal h, s, v;
    currColor.getHsvF(&h, &s, &v);
    color.setHsvF((qreal)x / width(),
                  1.0 - (qreal)y / height(),
                  v);
    return color;
}

QPoint ColorPickArea::colorToPoint(const QColor& color) const
{
    qreal h, s, v;
    color.getHsvF(&h, &s, &v);
    return QPoint(h * width(), (1.0 - s) * height());
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
    currColor = pointToColor(pos.x(), pos.y());
    emit colorChanged(currColor);
    repaint(QRect(event->pos() - QPoint(10, 10), event->pos() + QPoint(10, 10)));
}

void ColorPickWidgetAbstract::setColor(const QColor& color)
{
    currColor = color;
    repaint();
}

void ColorValueBar::paintEvent(QPaintEvent* event)
{
    QPainter p(this);
    QColor color = currColor;
    qreal h, s, v;
    currColor.getHsvF(&h, &s, &v);
    v = 1.0 - v;

    const int trianleSize = 10;
    QRect barRect = rect();
    barRect.setWidth(barRect.width() - trianleSize);


    for (int y = barRect.y(); y <= barRect.bottom(); y++) {
        color.setHsvF(h, s, 1.0 - (qreal)y / height());
        p.setPen(color);
        p.drawLine(barRect.x(), y, barRect.right(), y);
    }

    p.setPen(palette().alternateBase().color());
    p.drawRect(rect());

    QRectF triangleRect = QRectF(barRect.right(), v * height() - trianleSize / 2,
                                 trianleSize, trianleSize);

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
    qreal h, s, v;
    color.getHsvF(&h, &s, &v);
    color.setHsvF(h, s, 1.0 - (qreal)y / height());
    return color;
}

QPoint ColorValueBar::colorToPoint(const QColor& color) const
{
    qreal h, s, v;
    color.getHsvF(&h, &s, &v);
    return QPoint(rect().x(), (1.0 - v) * height());
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
    connect(this, &ColorPicker::colorChanged,
            ui->colorPickArea, &ColorPickArea::setColor);
    connect(this, &ColorPicker::colorChanged,
            ui->valuePickBar, &ColorValueBar::setColor);
    connect(this, &ColorPicker::colorChanged,
            ui->colorShow, &ColorShowWidget::setColor);

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
    if (!QRegExp("#[0-9a-fA-F]{6}").exactMatch(txt)) {
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
    return pixmap.toImage().pixelColor(0, 0);
}

bool ColorPicker::isPickingFromScreen() const
{
    return pickingFromScreen;
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

void ColorShowWidget::paintEvent(QPaintEvent* event)
{
    QPainter p(this);
    p.setPen(currColor);
    p.setBrush(QBrush(currColor));
    p.drawRect(event->rect());
    p.end();
}
