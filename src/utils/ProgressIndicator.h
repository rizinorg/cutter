
#ifndef PROGRESSINDICATOR_H
#define PROGRESSINDICATOR_H

#include <QWidget>

class ProgressIndicator: public QWidget
{
public:
    ProgressIndicator(QWidget *parent = nullptr);
    virtual ~ProgressIndicator();

    QSize minimumSizeHint() const override;
    QSize sizeHint() const override;

    bool getProgressIndicatorVisible() const    { return progressIndicatorVisible; }
    void setProgressIndicatorVisible(bool visible);

    bool getAnimating() const                   { return animating; }
    void setAnimating(bool animating);

protected:
    void timerEvent(QTimerEvent *event) override;
    void paintEvent(QPaintEvent *event) override;

private:
    bool animating = true;
    bool progressIndicatorVisible = true;

    int animationTimerId = 0;
    int animationState = 0;

    void updateAnimationTimer();
};


#endif //PROGRESSINDICATOR_H
