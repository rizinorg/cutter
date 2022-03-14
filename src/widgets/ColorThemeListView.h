#ifndef COLORTHEMELISTVIEW_H
#define COLORTHEMELISTVIEW_H

#include <QTimer>
#include <QListView>
#include <QJsonDocument>
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
public:
    ColorThemeListView(QWidget *parent = nullptr);
    virtual ~ColorThemeListView() override {}

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
public:
    ColorSettingsModel(QObject *parent = nullptr);
    virtual ~ColorSettingsModel() override {}

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
public:
    ColorOptionDelegate(QObject *parent = nullptr);
    ~ColorOptionDelegate() override {}

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
