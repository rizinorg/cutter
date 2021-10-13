#ifndef ADDRESSABLE_DOCK_WIDGET_H
#define ADDRESSABLE_DOCK_WIDGET_H

#include "CutterDockWidget.h"
#include "core/Cutter.h"

#include <QAction>

class CutterSeekable;

class AddressableDockWidget : public CutterDockWidget
{
    Q_OBJECT

#if QT_VERSION < QT_VERSION_CHECK(5, 13, 0)
#    define Q_DISABLE_COPY(AddressableDockWidget)                                                  \
        AddressableDockWidget(const AddressableDockWidget &m) = delete;                            \
        AddressableDockWidget &operator=(const AddressableDockWidget &m) = delete;

#    define Q_DISABLE_MOVE(AddressableDockWidget)                                                  \
        AddressableDockWidget(AddressableDockWidget &&m) = delete;                                 \
        AddressableDockWidget &operator=(AddressableDockWidget &&m) = delete;

#    define Q_DISABLE_COPY_MOVE(AddressableDockWidget)                                             \
        Q_DISABLE_COPY(AddressableDockWidget)                                                      \
        Q_DISABLE_MOVE(AddressableDockWidget)
#endif

    Q_DISABLE_COPY_MOVE(AddressableDockWidget)

public:
    AddressableDockWidget(MainWindow *parent);
    ~AddressableDockWidget() override;

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
