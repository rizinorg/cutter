#ifndef PREFERENCECATEGORY_H
#define PREFERENCECATEGORY_H

#include <QStackedWidget>
#include <QString>
#include <QTreeWidget>

/**
 * @brief Main class that divides the view into two sides in Preferences:
 *
 * Left Side: Tree containing items for all options widget
 * Right Side: Content for selected item
 */
class PreferenceCategory
{
public:
    PreferenceCategory(const QString &name, const QIcon &icon);
    PreferenceCategory(const QString &name, QWidget *widget, const QIcon &icon);
    PreferenceCategory(const QString &name, QWidget *widget, const QIcon &icon,
                       const QList<PreferenceCategory> &children);
    PreferenceCategory(const QString &name, const QIcon &icon,
                       const QList<PreferenceCategory> &children);

    void addItem(QTreeWidget &tree, QStackedWidget &panel);
    void addItem(QTreeWidgetItem &tree, QStackedWidget &panel);

private:
    QString name;
    QIcon icon;
    QWidget *widget;
    QList<PreferenceCategory> children;
};

#endif // PREFERENCECATEGORY_H
