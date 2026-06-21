#ifndef SEARCHABLEDOCKWIDGET_H
#define SEARCHABLEDOCKWIDGET_H

#include "CutterDockWidget.h"
#include "CutterSearchable.h"

class SearchBarWidget;

/**
 * @brief A dock widget that includes a search bar
 */
class CUTTER_EXPORT SearchableDockWidget : public CutterDockWidget, public CutterSearchableI
{
    Q_OBJECT

public:
    explicit SearchableDockWidget(MainWindow *parent);

    void updateSearchBarPosition();

protected:
    SearchBarWidget *searchBar;

    void resizeEvent(QResizeEvent *event) override;
    int searchHPadding() const override;
    int searchVPadding() const override;

private:
};

#endif // SEARCHABLEDOCKWIDGET_H
