#include "CutterSearchable.h"

#include "SearchBarWidget.h"
#include "shortcuts/ShortcutManager.h"

#include <QAbstractScrollArea>
#include <QObject>
#include <QScrollBar>

void CutterSearchableHelper::setupConnections(QWidget *parent, SearchBarWidget *searchBar)
{
    auto *searchable = dynamic_cast<CutterSearchableI *>(parent);
    if (!searchBar || !parent || !searchable) {
        return;
    }

    searchBar->hide();

    QObject::connect(searchBar, &SearchBarWidget::searchChanged, parent,
                     [searchable](const QString &text, int options) {
                         searchable->searchChanged(text, options);
                     });

    QObject::connect(searchBar, &SearchBarWidget::findNextTriggered, parent,
                     [searchable]() { searchable->findNext(); });

    QObject::connect(searchBar, &SearchBarWidget::findPrevTriggered, parent,
                     [searchable]() { searchable->findPrev(); });

    QObject::connect(searchBar, &SearchBarWidget::findLastTriggered, parent,
                     [searchable]() { searchable->findLast(); });

    QObject::connect(searchBar, &SearchBarWidget::hideTriggered, parent,
                     [searchable]() { searchable->searchBarHidden(); });

    QObject::connect(searchBar, &SearchBarWidget::showTriggered, parent,
                     [searchable]() { searchable->searchBarShown(); });

    QShortcut *shortcut = Shortcuts()->makeQShortcut("Search.toggle", parent);
    shortcut->setContext(Qt::WidgetWithChildrenShortcut);
    QObject::connect(shortcut, &QShortcut::activated, parent, [=]() {
        if (searchBar->isVisible()) {
            if (searchBar->hasFocus()) {
                searchBar->hideSearchBar();
            } else {
                searchBar->setFocus();
                searchBar->selectText();
            }
        } else {
            searchBar->showSearchBar();

            QWidget *searchArea = searchable->searchableArea();
            const int hPadding = searchable->searchHPadding();
            const int vPadding = searchable->searchVPadding();

            positionSearchBar(parent, searchBar, searchArea, hPadding, vPadding);
        }
    });
}

void CutterSearchableHelper::positionSearchBar(QWidget *parent, SearchBarWidget *searchBar,
                                               QWidget *searchArea, int hPadding, int vPadding)
{
    if (!searchBar || !searchArea) {
        return;
    }

    int searchBarWidth = 0;
    if (auto *scrollArea = qobject_cast<QAbstractScrollArea *>(searchArea)) {
        const QScrollBar *scrollBar = scrollArea->verticalScrollBar();
        searchBarWidth = (scrollBar && scrollBar->isVisible()) ? scrollBar->width() : 0;
    }

    const QPoint areaPos = searchArea->mapTo(parent, QPoint(0, 0));
    const int x =
            areaPos.x() + searchArea->width() - searchBarWidth - searchBar->width() - hPadding;
    const int y = areaPos.y() + vPadding;

    searchBar->move(x, y);
}
