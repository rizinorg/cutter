#ifndef CUTTERSEARCHABLE_H
#define CUTTERSEARCHABLE_H

#include "CutterCommon.h"

class QString;
class SearchBarWidget;
class QWidget;

enum SearchOption : ut8 {
    CaseSensitive = 1,
    WholeWords = 1 << 1,
    RegExp = 1 << 2,
    HighlightMatches = 1 << 3
};

/**
 * @brief Interface for any widget that needs a search bar
 */
class CutterSearchableI
{
public:
    virtual ~CutterSearchableI() = default;

    /**
     * @brief Widget which the search bar will be shown relative to
     */
    virtual QWidget *searchableArea() const = 0;

    /**
     * @brief Padding from the right
     */
    virtual int searchHPadding() const = 0;

    /**
     * @brief Padding from the top
     */
    virtual int searchVPadding() const = 0;

    /**
     * @brief Called when the user types in the search bar or changes search options
     * Text has a debounce timer of 200ms
     */
    virtual void searchChanged(const QString &text, int options) = 0;

    virtual void findNext() = 0;
    virtual void findPrev() = 0;
    virtual void findLast() = 0;

    /**
     * @brief Called when the search bar is closed
     */
    virtual void searchBarHidden() = 0;

    /**
     * @brief Called when the search bar is shown
     */
    virtual void searchBarShown() = 0;
};

/**
 * @brief Helper functions for the search bar
 */
namespace CutterSearchableHelper {

/**
 * @brief Connects the search bar to the parent widget
 * @param parent The widget that owns the search bar
 * @param bar The search bar to connect
 */
void setupConnections(QWidget *parent, SearchBarWidget *bar);

/**
 * @brief Sets the search bar position
 * Call this in the resizeEvent of the parent widget
 * @param parent The main widget holding the search bar
 * @param searchBar The search bar being moved
 * @param searchArea The widget which the search bar will be shown relative to
 * @param hPadding Padding from the right
 * @param vPadding Padding from the top
 */
void positionSearchBar(QWidget *parent, SearchBarWidget *searchBar, QWidget *searchArea,
                       int hPadding, int vPadding);
};

#endif // CUTTERSEARCHABLE_H
