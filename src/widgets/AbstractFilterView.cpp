#include "AbstractFilterView.h"

#include <QAction>
#include <QMenu>

AbstractFilterView::AbstractFilterView(QWidget *parent) : QWidget(parent) {}

void AbstractFilterView::setupSharedConnections()
{
    debounceTimer = new QTimer(this);
    debounceTimer->setSingleShot(true);

    connect(debounceTimer, &QTimer::timeout, this,
            [this]() { emit filterTextChanged(lineEdit()->text()); });

    connect(lineEdit(), &QLineEdit::textChanged, this, [this]() { debounceTimer->start(150); });
}

void AbstractFilterView::setItemCount(int count)
{
    lineEdit()->setItemCount(count);
}

void AbstractFilterView::showItemCount(bool show)
{
    lineEdit()->showItemCount(show);
}

void AbstractFilterView::showFilter()
{
    show();
    lineEdit()->setFocus();
}

void AbstractFilterView::clearFilter()
{
    lineEdit()->setText("");
}

void AbstractFilterView::closeFilter()
{
    lineEdit()->setText("");
    hide();
    emit filterClosed();
}

void AbstractFilterView::showCustomContextMenu(const QPoint &pos)
{
    const QWidget *child = this->childAt(pos);

    QMenu *menu;
    QAction *showItemCountAction;
    const QString showItemCountStr(tr("Show item count"));

    QAction *autoHideItemCountAction;
    const QString autoHideItemCountStr(tr("Hide item count on overflow"));

    if (child && (child == lineEdit() || child->parentWidget() == lineEdit())) {
        menu = lineEdit()->createStandardContextMenu();
        menu->addSeparator();

        QMenu *optionsMenu = menu->addMenu(tr("Options"));
        showItemCountAction = optionsMenu->addAction(showItemCountStr);
        autoHideItemCountAction = optionsMenu->addAction(autoHideItemCountStr);

    } else {
        menu = new QMenu(this);
        showItemCountAction = menu->addAction(showItemCountStr);
        autoHideItemCountAction = menu->addAction(autoHideItemCountStr);
    }

    showItemCountAction->setCheckable(true);
    showItemCountAction->setChecked(lineEdit()->itemCountVisible());
    connect(showItemCountAction, &QAction::triggered, this,
            [autoHideItemCountAction, this](bool checked) {
                showItemCount(checked);
                autoHideItemCountAction->setEnabled(checked);
            });

    autoHideItemCountAction->setCheckable(true);
    autoHideItemCountAction->setChecked(lineEdit()->itemCountAutoHide());
    autoHideItemCountAction->setEnabled(showItemCountAction->isChecked());
    connect(autoHideItemCountAction, &QAction::triggered, lineEdit(),
            &ItemCountLineEdit::setItemCountAutoHide);

    menu->exec(this->mapToGlobal(pos));

    delete menu;
}
