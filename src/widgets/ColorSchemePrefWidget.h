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


struct ColorOption {
    QString optionName;
    QString displayingText;
    QColor color;
};
Q_DECLARE_METATYPE(ColorOption);

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
    /**
     * @brief Shows color choose dialog and changes current color.
     * Triggered when ColorViewButton is clicked.
     */
    void changeCurrentColor();

    void colorOptionChanged(const ColorOption& option);

    void resetCurrentColor();


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
        return m_data.size();
    }

    void updateScheme();

    QJsonDocument getScheme() const;

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
    QColor standardTextColor;
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

    void dataChanged(const QModelIndex & topLeft, const QModelIndex & bottomRight,
                     const QVector<int> &roles = QVector<int>()) override;

signals:
    void indexChanged(const QModelIndex &ni);
    void colorOptionChanged(const ColorOption& option);

private:
    ColorOptionDelegate *delegate;
};

#endif // COLORSCHEMEPREFWIDGET_H
