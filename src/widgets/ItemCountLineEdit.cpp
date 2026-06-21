#include "ItemCountLineEdit.h"

#include "Configuration.h"

#include <QLabel>
#include <QResizeEvent>

namespace {
constexpr int padding = 5;
}

ItemCountLineEdit::ItemCountLineEdit(QWidget *parent)
    : QLineEdit(parent),
      itemCountLabel(new QLabel(this)),
      isItemCountVisible(Config()->getItemCountVisible()),
      itemCountAutoHideEnabled(Config()->getItemCountAutoHide())
{
    // parent widget must handle the context menu
    this->setContextMenuPolicy(Qt::NoContextMenu);

    itemCountLabel->setStyleSheet("QLabel { background: transparent; }");

    connect(this, &QLineEdit::textChanged, this, &ItemCountLineEdit::updateLabelPosition);
    connect(Config(), &Configuration::itemCountOptionsChanged, this,
            [this] { showItemCount(Config()->getItemCountVisible()); });
    connect(Config(), &Configuration::itemCountOptionsChanged, this,
            [this] { setItemCountAutoHide(Config()->getItemCountAutoHide()); });

    if (!isItemCountVisible) {
        itemCountLabel->hide();
    }
    updateLabelPosition();
}

void ItemCountLineEdit::setItemCount(int count)
{
    itemCountLabel->setText(QString("%1 Items").arg(count));
    updateLabelPosition();
}

bool ItemCountLineEdit::itemCountVisible() const
{
    return isItemCountVisible;
}

void ItemCountLineEdit::setItemCountAutoHide(bool value)
{
    itemCountAutoHideEnabled = value;
    updateLabelPosition();
}

bool ItemCountLineEdit::itemCountAutoHide() const
{
    return itemCountAutoHideEnabled;
}

void ItemCountLineEdit::showItemCount(bool show)
{
    isItemCountVisible = show;
    itemCountLabel->setVisible(show);
    if (!show) {
        this->setTextMargins(0, 0, 0, 0);
    }
    updateLabelPosition();
}

void ItemCountLineEdit::resizeEvent(QResizeEvent *event)
{
    QLineEdit::resizeEvent(event);
    updateLabelPosition();
}

void ItemCountLineEdit::updateLabelPosition()
{
    if (!isItemCountVisible) {
        return;
    }

    itemCountLabel->adjustSize();

    const int labelWidth = itemCountLabel->sizeHint().width();
    const int x = this->width() - labelWidth - padding;

    if (itemCountAutoHideEnabled) {
        const QFont font = this->font();
        const QFontMetrics fm(font);
#if QT_VERSION >= QT_VERSION_CHECK(5, 11, 0)
        const int textWidth = fm.horizontalAdvance(this->text());
#else
        int textWidth = fm.width(this->text());
#endif
        const int buffer = Config()->windowColorIsDark() ? 4 : 5;
        if (x <= (padding * buffer) + textWidth) {
            itemCountLabel->hide();
            this->setTextMargins(0, 0, 0, 0);
            return;
        }
    }

    // required so the label doesn't overlap with placeholder text
    auto margins = this->textMargins();
    margins.setRight(labelWidth + padding);
    this->setTextMargins(margins);

    const int y = (this->height() - itemCountLabel->height()) / 2;
    itemCountLabel->move(x, y);
    itemCountLabel->show();
}
