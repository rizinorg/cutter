#ifndef ADDRESSABLE_DOCK_WIDGET_H
#define ADDRESSABLE_DOCK_WIDGET_H

#include "CutterDockWidget.h"
#include "core/Cutter.h"

#include <QAction>

class CutterSeekable;

class AddressableDockWidget : public CutterDockWidget
{
    Q_OBJECT
public:
    AddressableDockWidget(MainWindow *parent);
    ~AddressableDockWidget() override {}

    CutterSeekable *getSeekable() const;

    QVariantMap serializeViewProprties() override;
    void deserializeViewProperties(const QVariantMap &properties) override;
public slots:
    void updateWindowTitle();

protected:
    CutterSeekable *seekable = nullptr;
    QAction syncAction;
    QMenu *dockMenu = nullptr;

    virtual QString getWindowTitle() const = 0;
    void contextMenuEvent(QContextMenuEvent *event) override;
};

#endif // ADDRESSABLE_DOCK_WIDGET_H
