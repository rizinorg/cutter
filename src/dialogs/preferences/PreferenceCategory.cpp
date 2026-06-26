#include "PreferenceCategory.h"

PreferenceCategory::PreferenceCategory(const QString &name, const QIcon &icon)
    : name(name), icon(icon), widget(nullptr), children {}
{
}

PreferenceCategory::PreferenceCategory(const QString &name, QWidget *widget, const QIcon &icon)
    : name(name), icon(icon), widget(widget), children {}
{
}

PreferenceCategory::PreferenceCategory(const QString &name, QWidget *widget, const QIcon &icon,
                                       const QList<PreferenceCategory> &children)
    : name(name), icon(icon), widget(widget), children(children)
{
}

PreferenceCategory::PreferenceCategory(const QString &name, const QIcon &icon,
                                       const QList<PreferenceCategory> &children)
    : name(name), icon(icon), widget(nullptr), children(children)
{
}

void PreferenceCategory::addItem(QTreeWidget &tree, QStackedWidget &panel)
{
    auto *w = new QTreeWidgetItem({ name });

    tree.addTopLevelItem(w);
    for (auto &c : children) {
        c.addItem(*w, panel);
    }

    w->setExpanded(true);
    w->setIcon(0, icon);

    // have some space from top and bottom
    w->setSizeHint(0, QSize(0, 22));

    if (widget) {
        panel.addWidget(widget);
        w->setData(0, Qt::UserRole, panel.count());
    }
}

void PreferenceCategory::addItem(QTreeWidgetItem &tree, QStackedWidget &panel)
{
    auto *w = new QTreeWidgetItem({ name });

    tree.addChild(w);
    for (auto &c : children) {
        c.addItem(*w, panel);
    }

    w->setExpanded(true);
    w->setIcon(0, icon);

    if (widget) {
        panel.addWidget(widget);
        w->setData(0, Qt::UserRole, panel.count());
    }
}
