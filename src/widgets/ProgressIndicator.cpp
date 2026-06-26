#include "ProgressIndicator.h"

#include <QPainter>

namespace {
const int lineWidth = 3;
const int paddingOuter = lineWidth + 2;
const int paddingInner = 8;
const int arms = 12;
const int timerInterval = 50;
}

ProgressIndicator::ProgressIndicator(QWidget *parent) : QWidget(parent)
{
    updateAnimationTimer();
}

ProgressIndicator::~ProgressIndicator() {}

void ProgressIndicator::setProgressIndicatorVisible(bool visible)
{
    const bool change = progressIndicatorVisible != visible;
    if (change) {
        progressIndicatorVisible = visible;
        update();
    }
    updateAnimationTimer();
}

void ProgressIndicator::setAnimating(bool animating)
{
    this->animating = animating;
    updateAnimationTimer();
}

void ProgressIndicator::updateAnimationTimer()
{
    const bool shouldBeAnimating = animating && progressIndicatorVisible;
    if (shouldBeAnimating && !animationTimerId) {
        animationTimerId = startTimer(timerInterval);
    } else {
        killTimer(animationTimerId);
        animationTimerId = 0;
    }
}

QSize ProgressIndicator::minimumSizeHint() const
{
    return QSize(16, 16);
}

QSize ProgressIndicator::sizeHint() const
{
    return QSize(32, 32);
}

void ProgressIndicator::timerEvent(QTimerEvent *)
{
    animationState = (animationState + 1) % arms;
    update();
}
void ProgressIndicator::paintEvent(QPaintEvent *)
{
    if (!getProgressIndicatorVisible()) {
        return;
    }

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);

    const QPen pen(palette().windowText(), lineWidth, Qt::SolidLine, Qt::RoundCap);
    painter.setPen(pen);

    const QPointF origin(width() * 0.5, height() * 0.5);
    const QLineF line(paddingInner, 0.0, width() * 0.5 - paddingOuter, 0.0);

    const qreal angle = 360.0 / arms;
    for (int i = 0; i < arms; i++) {
        const int state = (i + (arms - animationState)) % arms;
        painter.setOpacity((float)state / arms);
        painter.drawLine(line * QTransform().translate(origin.x(), origin.y()).rotate(angle * i));
    }
}
