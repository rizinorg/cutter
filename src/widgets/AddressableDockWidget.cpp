#include "AddressableDockWidget.h"
#include "common/CutterSeekable.h"
#include "MainWindow.h"
#include <QAction>
#include <QEvent>
#include <QMenu>
#include <QContextMenuEvent>

AddressableDockWidget::AddressableDockWidget(MainWindow *parent)
    : CutterDockWidget(parent)
    , seekable(new CutterSeekable(this))
    , syncAction(tr("Sync/unsync offset"), this)
{
    connect(seekable, &CutterSeekable::syncChanged, this, &AddressableDockWidget::updateWindowTitle);
    connect(&syncAction, &QAction::triggered, seekable, &CutterSeekable::toggleSynchronization);

    dockMenu = new QMenu(this);
    dockMenu->addAction(&syncAction);

    setContextMenuPolicy(Qt::ContextMenuPolicy::DefaultContextMenu);
}

QVariantMap AddressableDockWidget::serializeViewProprties()
{
    auto result = CutterDockWidget::serializeViewProprties();
    result["synchronized"] = seekable->isSynchronized();
    return result;
}

void AddressableDockWidget::deserializeViewProperties(const QVariantMap &properties)
{
    QVariant synchronized = properties.value("synchronized", true);
    seekable->setSynchronization(synchronized.toBool());
}

void AddressableDockWidget::updateWindowTitle()
{
    QString name = getWindowTitle();
    QString id = getDockNumber();
    if (!id.isEmpty()) {
        name += " " + id;
    }
    if (!seekable->isSynchronized()) {
        name += CutterSeekable::tr(" (unsynced)");
    }
    setWindowTitle(name);
}

void AddressableDockWidget::contextMenuEvent(QContextMenuEvent *event)
{
    event->accept();
    dockMenu->exec(mapToGlobal(event->pos()));
}

CutterSeekable *AddressableDockWidget::getSeekable() const
{
    return seekable;
}
