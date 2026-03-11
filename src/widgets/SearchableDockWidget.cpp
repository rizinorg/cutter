#include "SearchableDockWidget.h"
#include "SearchBarWidget.h"

#include <QScrollBar>
#include <QAbstractScrollArea>
#include <QTimer>

namespace {
constexpr int hPadding = 7;
constexpr int vPadding = 4;
};

SearchableDockWidget::SearchableDockWidget(MainWindow *parent)
    : CutterDockWidget(parent), m_searchBar(new SearchBarWidget(this))
{
    CutterSearchableHelper::setupConnections(this, m_searchBar);
}

void SearchableDockWidget::resizeEvent(QResizeEvent *event)
{
    CutterDockWidget::resizeEvent(event);
    updateSearchBarPosition();
}

void SearchableDockWidget::updateSearchBarPosition()
{
    CutterSearchableHelper::positionSearchBar(this, m_searchBar, searchableArea(), searchHPadding(),
                                              searchVPadding());
}

int SearchableDockWidget::searchHPadding() const
{
    return hPadding;
}

int SearchableDockWidget::searchVPadding() const
{
    return vPadding;
}
