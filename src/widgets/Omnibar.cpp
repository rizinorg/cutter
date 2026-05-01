#include "Omnibar.h"
#include "core/MainWindow.h"
#include "CutterSeekable.h"
#include "shortcuts/ShortcutManager.h"

#include <QStringListModel>
#include <QCompleter>
#include <QShortcut>
#include <QAbstractItemView>
#include <QStringListModel>
#include <QCollator>
#include <QStandardItemModel>
#include <QTimer>
#include <QEvent>
#include <QKeyEvent>

OmnibarCompleter::OmnibarCompleter(QObject *parent) : QCompleter(parent) {}

QStringList OmnibarCompleter::splitPath(const QString &) const
{
    return QStringList("");
}

QString OmnibarCompleter::pathFromIndex(const QModelIndex &index) const
{
    int type = index.data(Qt::UserRole).toInt();

    if (type == Omnibar::ShowMore || type == Omnibar::ShowAll) {
        if (auto edit = qobject_cast<QLineEdit *>(widget())) {
            return edit->text();
        }
    }

    return QCompleter::pathFromIndex(index);
}

Omnibar::Omnibar(MainWindow *main, QWidget *parent)
    : QLineEdit(parent), main(main), m_completerModel(new QStandardItemModel(this))
{
    // QLineEdit basic features
    this->setMinimumHeight(16);
    this->setFrame(false);
    this->setPlaceholderText(tr("Type flag name or address here"));
    this->setStyleSheet("border-radius: 5px; padding: 0 8px; margin: 5px 0;");
    this->setTextMargins(10, 0, 0, 0);
    this->setClearButtonEnabled(true);

    connect(this, &QLineEdit::textEdited, this, [this](const QString &text) {
        m_maxShownItems = Config()->getOmnibarEntriesCount(); // reset entries count to default
        handleSearch(text);
    });

    // Esc clears omnibar
    QShortcut *clear_shortcut = Shortcuts()->makeQShortcut("Omnibar.clear", this);
    connect(clear_shortcut, &QShortcut::activated, this, &Omnibar::clear);
    clear_shortcut->setContext(Qt::WidgetWithChildrenShortcut);

    // Set gotoEntry completer for jump history
    m_completer = new OmnibarCompleter(this);
    m_completer->setModel(m_completerModel);
    m_completer->setMaxVisibleItems(20);
    m_completer->setFilterMode(Qt::MatchContains);
    m_completer->setCompletionMode(QCompleter::PopupCompletion);
    m_completer->popup()->installEventFilter(this);
    m_completer->popup()->viewport()->installEventFilter(this);

    this->setCompleter(m_completer);
    this->installEventFilter(this);

    connect(Core(), &CutterCore::flagsChanged, this, &Omnibar::refresh);
    connect(Core(), &CutterCore::codeRebased, this, &Omnibar::refresh);
    connect(Core(), &CutterCore::refreshAll, this, &Omnibar::refresh);
}

bool Omnibar::eventFilter(QObject *obj, QEvent *event)
{
    QAbstractItemView *popup = m_completer->popup();

    if (((obj == popup || obj == this) && (event->type() == QEvent::KeyPress))
        || (obj == popup->viewport() && event->type() == QEvent::MouseButtonPress)) {

        QModelIndex index;

        if (event->type() == QEvent::MouseButtonPress) {
            QMouseEvent *mouseEvent = static_cast<QMouseEvent *>(event);
            if (mouseEvent->button() == Qt::LeftButton) {
                index = popup->indexAt(mouseEvent->pos());
            }
        } else if (event->type() == QEvent::KeyPress) {
            QKeyEvent *keyEvent = static_cast<QKeyEvent *>(event);

            int keyInt = keyEvent->modifiers() | keyEvent->key();
            QKeySequence eventSeq(keyInt);
            if (eventSeq == Shortcuts()->getKeySequence("Omnibar.showMore")) {
                handleShowMore(popup->currentIndex().row());
                return true;
            } else if (eventSeq == Shortcuts()->getKeySequence("Omnibar.showAll")) {
                handleShowAll(popup->currentIndex().row());
                return true;
            } else if (keyEvent->key() == Qt::Key_Enter || keyEvent->key() == Qt::Key_Return) {
                index = popup->currentIndex();
            }
        }

        if (index.isValid()) {
            int row = index.row();
            int type = index.data(Qt::UserRole).toInt();

            if (type == ItemType::ShowMore) {
                handleShowMore(row);
            } else if (type == ItemType::ShowAll) {
                handleShowAll(row);
            } else {
                goToEntry();
                popup->hide();
            }
            return true;
        }
    }
    return QObject::eventFilter(obj, event);
}

void Omnibar::refresh()
{
    m_flags.clear();
    const auto flags = Core()->getAllFlags();
    for (const auto &f : flags) {
        m_flags.append(f.name);
    }

    QCollator collator;
    collator.setNumericMode(true);
    collator.setCaseSensitivity(Qt::CaseInsensitive);
    std::sort(m_flags.begin(), m_flags.end(), [&collator](const QString &a, const QString &b) {
        return collator.compare(a, b) < 0;
    });

    handleSearch(this->text());
}

void Omnibar::clear()
{
    QLineEdit::clear();

    m_completerModel->clear();
    m_completer->popup()->hide();
}

void Omnibar::goToEntry()
{
    QString str = this->text();
    if (!str.isEmpty()) {
        if (auto memoryWidget = main->getLastMemoryWidget()) {
            RVA offset = Core()->math(str);
            memoryWidget->getSeekable()->seek(offset);
            memoryWidget->raiseMemoryWidget();
        } else {
            Core()->seekAndShow(str);
        }
    }

    this->setText("");
    this->clearFocus();
}

void Omnibar::handleSearch(const QString &Text, bool append)
{
    m_searchedText = Text;
    m_matchCount = 0;

    if (!append) {
        m_completerModel->clear();
        m_lastIndex = 0;
    } else {
        // remove "show more" and "show all" rows (last two)
        m_completerModel->removeRows(m_completerModel->rowCount() - 2, 2);
    }

    if (Text.isEmpty()) {
        return;
    }

    bool hasMore = false;
    int itemsInModel = m_completerModel->rowCount();

    for (int i = 0; i < m_flags.size(); ++i) {
        const QString &flag = m_flags[i];
        if (flag.contains(Text, Qt::CaseInsensitive)) {
            const bool showAll = !Config()->getOmnibarLimitEntries();
            if ((itemsInModel < m_maxShownItems) || showAll) {
                if (!append || i > m_lastIndex) {
                    QStandardItem *item = new QStandardItem(flag);
                    item->setData(ItemType::Standard, Qt::UserRole);
                    m_completerModel->appendRow(item);
                    ++itemsInModel;
                    m_lastIndex = i;
                }
            } else {
                hasMore = true;
            }
            ++m_matchCount;
        }
    }

    if (hasMore) {
        auto addActionRow = [&](const QString &text, ItemType type) {
            QStandardItem *actionItem = new QStandardItem(text);
            actionItem->setData(type, Qt::UserRole);

            QPalette palette = this->palette();
#if QT_VERSION >= QT_VERSION_CHECK(5, 12, 0)
            QColor placeholderColor = palette.color(QPalette::PlaceholderText);
#else
            QColor placeholderColor = palette.color(QPalette::Text);
            placeholderColor.setAlpha(128);
#endif
            actionItem->setForeground(placeholderColor);

            QFont font = actionItem->font();
            font.setItalic(true);
            actionItem->setFont(font);

            m_completerModel->appendRow(actionItem);
        };

        addActionRow(tr("Show More"), ItemType::ShowMore);
        addActionRow(tr("Show All (%1 of %2)").arg(m_maxShownItems).arg(m_matchCount),
                     ItemType::ShowAll);
    }

    m_completer->setCompletionPrefix("");
    m_completer->complete();
}

void Omnibar::handleShowMore(int currentRow)
{
    if (m_maxShownItems >= m_matchCount) {
        return;
    }
    m_maxShownItems =
            std::min(m_maxShownItems + Config()->getOmnibarEntriesIncrement(), m_matchCount);
    handleSearch(m_searchedText, true);
    m_completer->popup()->setCurrentIndex(m_completerModel->index(currentRow, 0));
}

void Omnibar::handleShowAll(int currentRow)
{
    if (m_maxShownItems >= m_matchCount) {
        return;
    }
    m_maxShownItems = m_matchCount;
    handleSearch(m_searchedText, true);
    m_completer->popup()->setCurrentIndex(m_completerModel->index(currentRow, 0));
}
