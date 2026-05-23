#include "AbstractFilterView.h"

#include "common/CutterSearchable.h"
#include "common/Helpers.h"

#include <QAction>
#include <QIcon>
#include <QMenu>

AbstractFilterView::AbstractFilterView(QWidget *parent) : QWidget(parent) {}

void AbstractFilterView::setupSharedConnections()
{
    auto *optionsMenu = new QMenu(this);
    caseSensitiveAction = optionsMenu->addAction(tr("&Case Sensitive"));
    caseSensitiveAction->setCheckable(true);

    wholeWordsAction = optionsMenu->addAction(tr("Exact Match"));
    wholeWordsAction->setCheckable(true);

    regexAction = optionsMenu->addAction(tr("Regular Expression"));
    regexAction->setCheckable(true);

    auto emitFilterChanged = [this]() {
        emit filterChanged(lineEdit()->text(), filterOptions());
        emit filterTextChanged(lineEdit()->text());
    };
    connect(caseSensitiveAction, &QAction::triggered, this, emitFilterChanged);
    connect(wholeWordsAction, &QAction::triggered, this, emitFilterChanged);
    connect(regexAction, &QAction::triggered, this, emitFilterChanged);

    auto *optionsAction = new QAction(this);
    const int lineEditHeight = lineEdit()->fontMetrics().height();
    const int targetIconSize = qRound(lineEditHeight * 0.8);
    const QIcon cogIcon(":/img/icons/cog_light.svg");
    const qreal dpr = qhelpers::devicePixelRatio(this);
    QPixmap pixmap = cogIcon.pixmap(qRound(targetIconSize * dpr), qRound(targetIconSize * dpr));
    pixmap.setDevicePixelRatio(dpr);
    optionsAction->setIcon(pixmap);
    optionsAction->setMenu(optionsMenu);
    lineEdit()->addAction(optionsAction, QLineEdit::LeadingPosition);

    debounceTimer = new QTimer(this);
    debounceTimer->setSingleShot(true);

    connect(debounceTimer, &QTimer::timeout, this, [this]() {
        emit filterChanged(lineEdit()->text(), filterOptions());
        emit filterTextChanged(lineEdit()->text());
    });

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

int AbstractFilterView::filterOptions() const
{
    int options = 0;
    if (caseSensitiveAction->isChecked()) {
        options |= SearchOption::CaseSensitive;
    }
    if (wholeWordsAction->isChecked()) {
        options |= SearchOption::WholeWords;
    }
    if (regexAction->isChecked()) {
        options |= SearchOption::RegExp;
    }
    return options;
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
