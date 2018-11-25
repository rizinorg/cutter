#ifndef COLORSCHEMEPREFWIDGET_H
#define COLORSCHEMEPREFWIDGET_H

#include <QFrame>
#include <QListView>
#include <QJsonObject>
#include <QAbstractListModel>
#include <QStyledItemDelegate>

namespace Ui {
class ColorSchemePrefWidget;
}

class ColorSchemePrefWidget : public QWidget
{
    Q_OBJECT
public:
    ColorSchemePrefWidget(QWidget *parent = nullptr);
    virtual ~ColorSchemePrefWidget();

public slots:
    void apply();

    void updateSchemeFromConfig();

private slots:
    void newColor();

    void indexChanged(const QModelIndex &ni);

private:
    Ui::ColorSchemePrefWidget *ui;
    bool isEditable;
};

//===============SERVICE STUFF BELOW===============

class ColorViewButton : public QFrame
{
    Q_OBJECT
public:
    ColorViewButton(QWidget *parent = nullptr);
    virtual ~ColorViewButton() override {}

public slots:
    void setColor(const QColor &c);

protected slots:
    void mouseReleaseEvent(QMouseEvent *event) override;

signals:
    void clicked();
};

struct ColorOption {
    QString optionName;
    QString displayingText;
    QColor color;
};
Q_DECLARE_METATYPE(ColorOption);


class ColorSettingsModel : public QAbstractListModel
{
    Q_OBJECT
public:
    ColorSettingsModel(QObject *parent = nullptr);
    virtual ~ColorSettingsModel() override {}

    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;

    void setColor(const QString &option, const QColor &color);

    QColor getBackroundColor() const;

    QColor getTextColor() const;

    int rowCount(const QModelIndex &parent = QModelIndex()) const override
    {
        Q_UNUSED(parent)
        return m_data.size();
    }

    void updateScheme();

private:
    QList<ColorOption> m_data;
};

class ColorOptionDelegate : public QStyledItemDelegate
{
    Q_OBJECT
public:
    ColorOptionDelegate(QObject *parent = nullptr) : QStyledItemDelegate (parent) {}
    ~ColorOptionDelegate() override {}

    void paint(QPainter *painter,
               const QStyleOptionViewItem &option,
               const QModelIndex &index) const override;

    void setBackgroundColor(const QColor &c);

    void setTextColor(const QColor &c);

private:
    QColor backgroundColor;
    QColor textColor;
};

class PreferencesListView : public QListView
{
    Q_OBJECT
public:
    PreferencesListView(QWidget *parent = nullptr);
    virtual ~PreferencesListView() override {}

    void setStandardColors();

protected slots:
    void currentChanged(const QModelIndex &current,
                        const QModelIndex &previous) override;

signals:
    void indexChanged(const QModelIndex &ni);

private:
    ColorOptionDelegate *delegate;
};

#endif // COLORSCHEMEPREFWIDGET_H
