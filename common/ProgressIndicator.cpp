
#include "ProgressIndicator.h"

#include <QPainter>

static const int lineWidth = 3;
static const int paddingOuter = lineWidth + 2;
static const int paddingInner = 8;
static const int arms = 12;
static const int timerInterval = 50;

ProgressIndicator::ProgressIndicator(QWidget *parent)
    : QWidget(parent)
{
    updateAnimationTimer();
}

ProgressIndicator::~ProgressIndicator()
{
}

void ProgressIndicator::setProgressIndicatorVisible(bool visible)
{
    bool change = progressIndicatorVisible != visible;
    progressIndicatorVisible = visible;
    if (change) {
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
    bool shouldBeAnimating = animating && progressIndicatorVisible;
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

    QPen pen(palette().windowText(), lineWidth, Qt::SolidLine, Qt::RoundCap);
    painter.setPen(pen);

    QPointF origin(width() * 0.5, height() * 0.5);
    QLineF line(paddingInner, 0.0, width() * 0.5 - paddingOuter, 0.0);

    qreal angle = 360.0 / arms;
    for (int i=0; i<arms; i++) {
        int state = (i + (arms - animationState)) % arms;
        painter.setOpacity((float)state / arms);
        painter.drawLine(line * QTransform()
            .translate(origin.x(), origin.y())
            .rotate(angle * i));
    }
}


