#ifndef COLORSCHEMEPREFWIDGET_H
#define COLORSCHEMEPREFWIDGET_H

#include <QFrame>
#include <QListView>
#include <QAbstractListModel>
#include <QStyledItemDelegate>
#include <QJsonObject>

constexpr const char *simpleTextOptionName = "text";
constexpr const char *defaultBackgroundOptionName = "background";
constexpr const char *path = "/home/optizone/git/cutter/src/themes/schemes/ColorScheme.json";

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

private slots:
    void on_setDefBack_clicked();
    void on_setDefFore_clicked();

    void newColor(const QColor &color);

    void indexChanged(const QModelIndex &ni);

private:
    Ui::ColorSchemePrefWidget *ui;
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
    void newColor(const QColor &nc);
};

struct ColorOption {
    QString optionName;
    QColor textColor;
    QColor backgroundColor;
};
Q_DECLARE_METATYPE(ColorOption);

class ColorSettingsModel : public QAbstractListModel
{
    Q_OBJECT
public:
    ColorSettingsModel(const QJsonObject &pref, QObject *parent = nullptr);
    virtual ~ColorSettingsModel() override {}

    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;

    void setColor(const QString &option,
                  const QString &textOrBack,
                  const QColor &color);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override { return m_data.size(); }

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
};

class PreferencesListView : public QListView
{
    Q_OBJECT
public:
    PreferencesListView(QWidget *parent = nullptr);
    virtual ~PreferencesListView() override {}

protected slots:
    void currentChanged(const QModelIndex &current,
                        const QModelIndex &previous) override;

signals:
    void indexChanged(const QModelIndex &ni);
};

#endif // COLORSCHEMEPREFWIDGET_H
