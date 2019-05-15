#ifndef COLORPICKER_H
#define COLORPICKER_H

#include <QWidget>

/**
 * @namespace ColorPickerHelpers is a namespace that hides all classes needed for ColorPicker class,
 * because classes inherite QObject can not be declared in *.cpp files or inside of another class.
 */
namespace ColorPickerHelpers {
class ColorPickWidgetAbstract : public QWidget
{
    Q_OBJECT
public:
    ColorPickWidgetAbstract(QWidget *parent = nullptr): QWidget(parent) {}
    virtual ~ColorPickWidgetAbstract() {}

signals:
    void colorChanged(const QColor& color);

public slots:
    virtual void setColor(const QColor& color) = 0;

protected:
    QColor currColor;
};
}

namespace Ui {
class ColorPicker;
}

/**
 * @brief The ColorPicker class provides widget that allows user to pick color
 * from screen or from palette or type in HSV or RGB or HEX representation of color.
 */
class ColorPicker : public ColorPickerHelpers::ColorPickWidgetAbstract
{
    Q_OBJECT
public:
    explicit ColorPicker(QWidget *parent = nullptr);
    ~ColorPicker();

    /**
     * @brief isPickingFromScreen returns true if color picker is picking from screen.
     */
    bool isPickingFromScreen() const;

    void setAlphaEnabled(bool enabled);


public slots:
    /**
     * @brief setColor sets displayed color to @a color and emits colorChanged signal.
     */
    virtual void setColor(const QColor &color) override;

    void colorChannelChanged();

    /**
     * @brief updateColor sets displayed color to @a color.
     */
    void updateColor(const QColor& color);

    /**
     * @brief startPickingFromScreen starts process of picking from screen.
     * Function is called automatically when "Pick from screen" button is clicked.
     */
    void startPickingFromScreen();

    /**
     * @brief stopPickingFromScreen terminates process of picking from screen.
     */
    void stopPickingFromScreen();

protected:
    void mouseReleaseEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;

private:
    Ui::ColorPicker *ui;
    bool pickingFromScreen;

    QColor getColorAtMouse();

    /**
     * @brief bufferColor is used to buffer current color while picking from screen.
     */
    QColor bufferColor;
};

namespace ColorPickerHelpers {
/**
 * @brief The ColorPickerWidget class is parent class for ColorPickArea and ColorValueBar classes.
 */
class ColorPickerWidget : public ColorPickWidgetAbstract {
    Q_OBJECT
public:
    ColorPickerWidget(QWidget *parent = nullptr);

protected:
    void mouseReleaseEvent(QMouseEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;

    virtual void mouseEvent(QMouseEvent* event);

    /**
     * @brief pointToColor converts coordinates on widget to color these coordinates represents.
     */
    virtual QColor pointToColor(int x, int y) const = 0;

    /**
     * @brief colorToPoint converts color to coordinates that represent this color.
     */
    virtual QPoint colorToPoint(const QColor& color) const = 0;
};

class ColorShowWidget : public ColorPickWidgetAbstract
{
    Q_OBJECT
public:
    explicit ColorShowWidget(QWidget *parent = nullptr);

    void setColor(const QColor& c) override;

protected:
    void paintEvent(QPaintEvent *event) override;
};

/**
 * @brief The ColorPickArea class provides widget that helps to pick
 * Saturation and Hue of color in HSV colorspace.
 */
class ColorPickArea : public ColorPickerWidget
{
    Q_OBJECT
public:
    explicit ColorPickArea(QWidget *parent = nullptr);

    void setColor(const QColor& c) override;

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    QColor pointToColor(int x, int y) const override;

    QPoint colorToPoint(const QColor& color) const override;
};

class AlphaChannelBar : public ColorPickerWidget
{
    Q_OBJECT
public:
    AlphaChannelBar(QWidget *parent = nullptr) : ColorPickerWidget(parent) {}

    void setColor(const QColor& c) override;

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    QColor pointToColor(int x, int y) const override;

    QPoint colorToPoint(const QColor& color) const override;
};

/**
 * @brief The ColorValueBar class provides widget that helps to set Valuse of color
 * in HSV colorspace.
 */
class ColorValueBar : public ColorPickerWidget
{
    Q_OBJECT
public:
    ColorValueBar(QWidget *parent = nullptr) : ColorPickerWidget(parent) {}

    void setColor(const QColor& c) override;

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    QColor pointToColor(int x, int y) const override;

    QPoint colorToPoint(const QColor& color) const override;
};
}

#endif // COLORPICKER_H
