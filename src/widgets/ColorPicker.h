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

#if QT_VERSION < QT_VERSION_CHECK(5, 13, 0)
#    define Q_DISABLE_COPY(ColorPickWidgetAbstract)                                                \
        ColorPickWidgetAbstract(const ColorPickWidgetAbstract &m) = delete;                        \
        ColorPickWidgetAbstract &operator=(const ColorPickWidgetAbstract &m) = delete;

#    define Q_DISABLE_MOVE(ColorPickWidgetAbstract)                                                \
        ColorPickWidgetAbstract(ColorPickWidgetAbstract &&m) = delete;                             \
        ColorPickWidgetAbstract &operator=(ColorPickWidgetAbstract &&m) = delete;

#    define Q_DISABLE_COPY_MOVE(ColorPickWidgetAbstract)                                           \
        Q_DISABLE_COPY(ColorPickWidgetAbstract)                                                    \
        Q_DISABLE_MOVE(ColorPickWidgetAbstract)
#endif

    Q_DISABLE_COPY_MOVE(ColorPickWidgetAbstract)

public:
    ColorPickWidgetAbstract(QWidget *parent = nullptr) : QWidget(parent) {}
    ~ColorPickWidgetAbstract() override = default;

signals:
    void colorChanged(const QColor &color);

public slots:
    virtual void setColor(const QColor &color) = 0;

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

#if QT_VERSION < QT_VERSION_CHECK(5, 13, 0)
#    define Q_DISABLE_COPY(ColorPicker)                                                            \
        ColorPicker(const ColorPicker &w) = delete;                                                \
        ColorPicker &operator=(const ColorPicker &w) = delete;

#    define Q_DISABLE_MOVE(ColorPicker)                                                            \
        ColorPicker(ColorPicker &&w) = delete;                                                     \
        ColorPicker &operator=(ColorPicker &&w) = delete;

#    define Q_DISABLE_COPY_MOVE(ColorPicker)                                                       \
        Q_DISABLE_COPY(ColorPicker)                                                                \
        Q_DISABLE_MOVE(ColorPicker)
#endif

    Q_DISABLE_COPY_MOVE(ColorPicker)

public:
    explicit ColorPicker(QWidget *parent = nullptr);
    ~ColorPicker() override;

    /**
     * @brief isPickingFromScreen returns true if color picker is picking from screen.
     */
    bool isPickingFromScreen() const;

    void setAlphaEnabled(bool enabled);

public slots:
    /**
     * @brief setColor sets displayed color to @a color and emits colorChanged signal.
     */
    void setColor(const QColor &color) override;

    void colorChannelChanged();

    /**
     * @brief updateColor sets displayed color to @a color.
     */
    void updateColor(const QColor &color);

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
    void mouseReleaseEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;

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
class ColorPickerWidget : public ColorPickWidgetAbstract
{
    Q_OBJECT

#if QT_VERSION < QT_VERSION_CHECK(5, 13, 0)
#    define Q_DISABLE_COPY(ColorPickerWidget)                                                      \
        ColorPickerWidget(const ColorPickerWidget &w) = delete;                                    \
        ColorPickerWidget &operator=(const ColorPickerWidget &w) = delete;

#    define Q_DISABLE_MOVE(ColorPickerWidget)                                                      \
        ColorPickerWidget(ColorPickerWidget &&w) = delete;                                         \
        ColorPickerWidget &operator=(ColorPickerWidget &&w) = delete;

#    define Q_DISABLE_COPY_MOVE(ColorPickerWidget)                                                 \
        Q_DISABLE_COPY(ColorPickerWidget)                                                          \
        Q_DISABLE_MOVE(ColorPickerWidget)
#endif

    Q_DISABLE_COPY_MOVE(ColorPickerWidget)

public:
    ColorPickerWidget(QWidget *parent = nullptr);
    ~ColorPickerWidget() override;

protected:
    void mouseReleaseEvent(QMouseEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;

    virtual void mouseEvent(QMouseEvent *event);

    /**
     * @brief pointToColor converts coordinates on widget to color these coordinates represents.
     */
    virtual QColor pointToColor(int x, int y) const = 0;

    /**
     * @brief colorToPoint converts color to coordinates that represent this color.
     */
    virtual QPoint colorToPoint(const QColor &color) const = 0;
};

class ColorShowWidget : public ColorPickWidgetAbstract
{
    Q_OBJECT

#if QT_VERSION < QT_VERSION_CHECK(5, 13, 0)
#    define Q_DISABLE_COPY(ColorShowWidget)                                                        \
        ColorShowWidget(const ColorShowWidget &w) = delete;                                        \
        ColorShowWidget &operator=(const ColorShowWidget &w) = delete;

#    define Q_DISABLE_MOVE(ColorShowWidget)                                                        \
        ColorShowWidget(ColorShowWidget &&w) = delete;                                             \
        ColorShowWidget &operator=(ColorShowWidget &&w) = delete;

#    define Q_DISABLE_COPY_MOVE(ColorShowWidget)                                                   \
        Q_DISABLE_COPY(ColorShowWidget)                                                            \
        Q_DISABLE_MOVE(ColorShowWidget)
#endif

    Q_DISABLE_COPY_MOVE(ColorShowWidget)

public:
    explicit ColorShowWidget(QWidget *parent = nullptr);
    ~ColorShowWidget() override;

    void setColor(const QColor &c) override;

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

#if QT_VERSION < QT_VERSION_CHECK(5, 13, 0)
#    define Q_DISABLE_COPY(ColorPickArea)                                                          \
        ColorPickArea(const ColorPickArea &w) = delete;                                            \
        ColorPickArea &operator=(const ColorPickArea &w) = delete;

#    define Q_DISABLE_MOVE(ColorPickArea)                                                          \
        ColorPickArea(ColorPickArea &&w) = delete;                                                 \
        ColorPickArea &operator=(ColorPickArea &&w) = delete;

#    define Q_DISABLE_COPY_MOVE(ColorPickArea)                                                     \
        Q_DISABLE_COPY(ColorPickArea)                                                              \
        Q_DISABLE_MOVE(ColorPickArea)
#endif

    Q_DISABLE_COPY_MOVE(ColorPickArea)

public:
    explicit ColorPickArea(QWidget *parent = nullptr);
    ~ColorPickArea() override;

    void setColor(const QColor &c) override;

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    QColor pointToColor(int x, int y) const override;

    QPoint colorToPoint(const QColor &color) const override;
};

class AlphaChannelBar : public ColorPickerWidget
{
    Q_OBJECT

#if QT_VERSION < QT_VERSION_CHECK(5, 13, 0)
#    define Q_DISABLE_COPY(AlphaChannelBar)                                                        \
        AlphaChannelBar(const AlphaChannelBar &w) = delete;                                        \
        AlphaChannelBar &operator=(const AlphaChannelBar &w) = delete;

#    define Q_DISABLE_MOVE(AlphaChannelBar)                                                        \
        AlphaChannelBar(AlphaChannelBar &&w) = delete;                                             \
        AlphaChannelBar &operator=(AlphaChannelBar &&w) = delete;

#    define Q_DISABLE_COPY_MOVE(AlphaChannelBar)                                                   \
        Q_DISABLE_COPY(AlphaChannelBar)                                                            \
        Q_DISABLE_MOVE(AlphaChannelBar)
#endif

    Q_DISABLE_COPY_MOVE(AlphaChannelBar)

public:
    AlphaChannelBar(QWidget *parent = nullptr) : ColorPickerWidget(parent) {}
    ~AlphaChannelBar() override = default;

    void setColor(const QColor &c) override;

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    QColor pointToColor(int x, int y) const override;

    QPoint colorToPoint(const QColor &color) const override;
};

/**
 * @brief The ColorValueBar class provides widget that helps to set Valuse of color
 * in HSV colorspace.
 */
class ColorValueBar : public ColorPickerWidget
{
    Q_OBJECT

#if QT_VERSION < QT_VERSION_CHECK(5, 13, 0)
#    define Q_DISABLE_COPY(ColorValueBar)                                                          \
        ColorValueBar(const ColorValueBar &w) = delete;                                            \
        ColorValueBar &operator=(const ColorValueBar &w) = delete;

#    define Q_DISABLE_MOVE(ColorValueBar)                                                          \
        ColorValueBar(ColorValueBar &&w) = delete;                                                 \
        ColorValueBar &operator=(ColorValueBar &&w) = delete;

#    define Q_DISABLE_COPY_MOVE(ColorValueBar)                                                     \
        Q_DISABLE_COPY(ColorValueBar)                                                              \
        Q_DISABLE_MOVE(ColorValueBar)
#endif

    Q_DISABLE_COPY_MOVE(ColorValueBar)

public:
    ColorValueBar(QWidget *parent = nullptr) : ColorPickerWidget(parent) {}
    ~ColorValueBar() override = default;

    void setColor(const QColor &c) override;

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    QColor pointToColor(int x, int y) const override;

    QPoint colorToPoint(const QColor &color) const override;
};
}

#endif // COLORPICKER_H
