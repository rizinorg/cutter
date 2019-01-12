
#include "RefreshDeferrer.h"
#include "widgets/CutterDockWidget.h"

RefreshDeferrer::RefreshDeferrer(RefreshDeferrerAccumulator *acc, QObject *parent) : QObject(parent),
    acc(acc)
{
}

RefreshDeferrer::~RefreshDeferrer()
{
    delete acc;
}

bool RefreshDeferrer::attemptRefresh(RefreshDeferrerParams params)
{
    if (dockWidget->isVisibleToUser()) {
        return true;
    } else {
        dirty = true;
        acc->accumulate(params);
        return false;
    }
}

void RefreshDeferrer::registerFor(CutterDockWidget *dockWidget)
{
    this->dockWidget = dockWidget;
    connect(dockWidget, &CutterDockWidget::becameVisibleToUser, this, [this]() {
        if(dirty) {
            emit refreshNow(acc->result());
            acc->clear();
            dirty = false;
        }
    });
}

