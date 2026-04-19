#include "ItemCountLineEdit.h"
#include "Configuration.h"

#include <QResizeEvent>
#include <QLabel>

namespace {
constexpr int PADDING = 5;
}

ItemCountLineEdit::ItemCountLineEdit(QWidget *parent)
    : QLineEdit(parent), m_itemCountLabel(new QLabel(this))
{
    // parent widget must handle the context menu
    this->setContextMenuPolicy(Qt::NoContextMenu);

    m_itemCountLabel->setStyleSheet("QLabel { background: transparent; }");

    connect(this, &QLineEdit::textChanged, this, &ItemCountLineEdit::updateLabelPosition);
    connect(Config(), &Configuration::itemCountOptionsChanged, this,
            [this] { showItemCount(Config()->getItemCountVisible()); });
    connect(Config(), &Configuration::itemCountOptionsChanged, this,
            [this] { setItemCountAutoHide(Config()->getItemCountAutoHide()); });

    m_itemCountAutoHide = Config()->getItemCountAutoHide();
    m_itemCountVisible = Config()->getItemCountVisible();
    if (!m_itemCountVisible) {
        m_itemCountLabel->hide();
    }
    updateLabelPosition();
}

void ItemCountLineEdit::setItemCount(int count)
{
    m_itemCountLabel->setText(QString("%1 Items").arg(count));
    updateLabelPosition();
}

bool ItemCountLineEdit::itemCountVisible() const
{
    return m_itemCountVisible;
}

void ItemCountLineEdit::setItemCountAutoHide(bool value)
{
    m_itemCountAutoHide = value;
    updateLabelPosition();
}

bool ItemCountLineEdit::itemCountAutoHide() const
{
    return m_itemCountAutoHide;
}

void ItemCountLineEdit::showItemCount(bool show)
{
    m_itemCountVisible = show;
    m_itemCountLabel->setVisible(show);
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
    if (!m_itemCountVisible) {
        return;
    }

    m_itemCountLabel->adjustSize();

    int labelWidth = m_itemCountLabel->sizeHint().width();
    int x = this->width() - labelWidth - PADDING;

    if (m_itemCountAutoHide) {
        QFont font = this->font();
        QFontMetrics fm(font);
#if QT_VERSION >= QT_VERSION_CHECK(5, 11, 0)
        int textWidth = fm.horizontalAdvance(this->text());
#else
        int textWidth = fm.width(this->text());
#endif
        const int BUFFER = Config()->windowColorIsDark() ? 4 : 5;
        if (x <= (PADDING * BUFFER) + textWidth) {
            m_itemCountLabel->hide();
            this->setTextMargins(0, 0, 0, 0);
            return;
        }
    }

    // required so the label doesn't overlap with placeholder text
    auto margins = this->textMargins();
    margins.setRight(labelWidth + PADDING);
    this->setTextMargins(margins);

    int y = (this->height() - m_itemCountLabel->height()) / 2;
    m_itemCountLabel->move(x, y);
    m_itemCountLabel->show();
}
