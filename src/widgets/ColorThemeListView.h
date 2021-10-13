#ifndef COLORTHEMELISTVIEW_H
#define COLORTHEMELISTVIEW_H

#include <QTimer>
#include <QListView>
#include <QJsonObject>
#include <QAbstractListModel>
#include <QStyledItemDelegate>

struct ColorOption
{
    QString optionName;
    QColor color;
    bool changed;
};
Q_DECLARE_METATYPE(ColorOption);

class ColorSettingsModel;

class ColorThemeListView : public QListView
{
    Q_OBJECT

#if QT_VERSION < QT_VERSION_CHECK(5, 13, 0)
#    define Q_DISABLE_COPY(ColorThemeListView)                                                     \
        ColorThemeListView(const ColorThemeListView &m) = delete;                                  \
        ColorThemeListView &operator=(const ColorThemeListView &m) = delete;

#    define Q_DISABLE_MOVE(ColorThemeListView)                                                     \
        ColorThemeListView(ColorThemeListView &&m) = delete;                                       \
        ColorThemeListView &operator=(ColorThemeListView &&m) = delete;

#    define Q_DISABLE_COPY_MOVE(ColorThemeListView)                                                \
        Q_DISABLE_COPY(ColorThemeListView)                                                         \
        Q_DISABLE_MOVE(ColorThemeListView)
#endif

    Q_DISABLE_COPY_MOVE(ColorThemeListView)

public:
    ColorThemeListView(QWidget *parent = nullptr);
    ~ColorThemeListView() override;

    ColorSettingsModel *colorSettingsModel() const;

protected slots:
    void currentChanged(const QModelIndex &current, const QModelIndex &previous) override;

    void dataChanged(const QModelIndex &topLeft, const QModelIndex &bottomRight,
                     const QVector<int> &roles = QVector<int>()) override;

    void mouseReleaseEvent(QMouseEvent *e) override;

    void mouseMoveEvent(QMouseEvent *e) override;

private slots:
    void blinkTimeout();

signals:
    void itemChanged(const QColor &option);

    void dataChanged(const ColorOption &data);

    void blink();

private:
    QTimer blinkTimer;
    QColor backgroundColor;
};

//==============================================

class ColorSettingsModel : public QAbstractListModel
{
    Q_OBJECT

#if QT_VERSION < QT_VERSION_CHECK(5, 13, 0)
#    define Q_DISABLE_COPY(ColorSettingsModel)                                                     \
        ColorSettingsModel(const ColorSettingsModel &w) = delete;                                  \
        ColorSettingsModel &operator=(const ColorSettingsModel &w) = delete;

#    define Q_DISABLE_MOVE(ColorSettingsModel)                                                     \
        ColorSettingsModel(ColorSettingsModel &&w) = delete;                                       \
        ColorSettingsModel &operator=(ColorSettingsModel &&w) = delete;

#    define Q_DISABLE_COPY_MOVE(ColorSettingsModel)                                                \
        Q_DISABLE_COPY(ColorSettingsModel)                                                         \
        Q_DISABLE_MOVE(ColorSettingsModel)
#endif

    Q_DISABLE_COPY_MOVE(ColorSettingsModel)

public:
    ColorSettingsModel(QObject *parent = nullptr);
    ~ColorSettingsModel() override;

    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;

    bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole) override;

    int rowCount(const QModelIndex &parent = QModelIndex()) const override
    {
        Q_UNUSED(parent)
        return theme.size();
    }

    void updateTheme();

    QJsonDocument getTheme() const;

private:
    QList<ColorOption> theme;
};

class ColorOptionDelegate : public QStyledItemDelegate
{
    Q_OBJECT

#if QT_VERSION < QT_VERSION_CHECK(5, 13, 0)
#    define Q_DISABLE_COPY(ColorOptionDelegate)                                                    \
        ColorOptionDelegate(const ColorOptionDelegate &d) = delete;                                \
        ColorOptionDelegate &operator=(const ColorOptionDelegate &d) = delete;

#    define Q_DISABLE_MOVE(ColorOptionDelegate)                                                    \
        ColorOptionDelegate(ColorOptionDelegate &&d) = delete;                                     \
        ColorOptionDelegate &operator=(ColorOptionDelegate &&d) = delete;

#    define Q_DISABLE_COPY_MOVE(ColorOptionDelegate)                                               \
        Q_DISABLE_COPY(ColorOptionDelegate)                                                        \
        Q_DISABLE_MOVE(ColorOptionDelegate)
#endif

    Q_DISABLE_COPY_MOVE(ColorOptionDelegate)

public:
    ColorOptionDelegate(QObject *parent = nullptr);
    ~ColorOptionDelegate() override;

    void paint(QPainter *painter, const QStyleOptionViewItem &option,
               const QModelIndex &index) const override;

    QSize sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const override;

    QRect getResetButtonRect() const;

private:
    const int margin = 12;
    QPixmap resetButtonPixmap;
    QRect resetButtonRect;

    QPixmap getPixmapFromSvg(const QString &fileName, const QColor &after) const;
};

#endif // COLORTHEMELISTVIEW_H
