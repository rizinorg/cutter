#include "Omnibar.h"

#include "CutterDescriptions.h"
#include "CutterSeekable.h"
#include "core/MainWindow.h"
#include "shortcuts/ShortcutManager.h"

#include <QAbstractItemView>
#include <QCollator>
#include <QCompleter>
#include <QEvent>
#include <QKeyEvent>
#include <QList>
#include <QShortcut>
#include <QStandardItemModel>
#include <QStringListModel>
#include <QTimer>

OmnibarCompleter::OmnibarCompleter(QObject *parent) : QCompleter(parent) {}

QStringList OmnibarCompleter::splitPath(const QString &) const
{
    return QStringList("");
}

QString OmnibarCompleter::pathFromIndex(const QModelIndex &index) const
{
    const int type = index.data(Qt::UserRole).toInt();

    if (type == Omnibar::ShowMore || type == Omnibar::ShowAll) {
        if (auto edit = qobject_cast<QLineEdit *>(widget())) {
            return edit->text();
        }
    }

    return QCompleter::pathFromIndex(index);
}

Omnibar::Omnibar(MainWindow *main, QWidget *parent)
    : QLineEdit(parent), main(main), completerModel(new QStandardItemModel(this))
{
    // QLineEdit basic features
    this->setMinimumHeight(16);
    this->setFrame(false);
    this->setPlaceholderText(tr("Type flag name or address here"));
    this->setStyleSheet("border-radius: 5px; padding: 0 8px; margin: 5px 0;");
    this->setTextMargins(10, 0, 0, 0);
    this->setClearButtonEnabled(true);

    connect(this, &QLineEdit::textEdited, this, [this](const QString &text) {
        maxShownItems = Config()->getOmnibarEntriesCount(); // reset entries count to default
        handleSearch(text);
    });

    // Esc clears omnibar
    QShortcut *clearShortcut = Shortcuts()->makeQShortcut("Omnibar.clear", this);
    connect(clearShortcut, &QShortcut::activated, this, &Omnibar::clear);
    clearShortcut->setContext(Qt::WidgetWithChildrenShortcut);

    // Set gotoEntry completer for jump history
    completer = new OmnibarCompleter(this);
    completer->setModel(completerModel);
    completer->setMaxVisibleItems(20);
    completer->setFilterMode(Qt::MatchContains);
    completer->setCompletionMode(QCompleter::PopupCompletion);
    completer->popup()->installEventFilter(this);
    completer->popup()->viewport()->installEventFilter(this);

    this->setCompleter(completer);
    this->installEventFilter(this);

    connect(Core(), &CutterCore::flagsChanged, this, &Omnibar::refresh);
    connect(Core(), &CutterCore::codeRebased, this, &Omnibar::refresh);
    connect(Core(), &CutterCore::refreshAll, this, &Omnibar::refresh);
}

bool Omnibar::eventFilter(QObject *obj, QEvent *event)
{
    QAbstractItemView *popup = completer->popup();

    if (((obj == popup || obj == this) && (event->type() == QEvent::KeyPress))
        || (obj == popup->viewport() && event->type() == QEvent::MouseButtonPress)) {

        QModelIndex index;

        if (event->type() == QEvent::MouseButtonPress) {
            const QMouseEvent *mouseEvent = static_cast<QMouseEvent *>(event);
            if (mouseEvent->button() == Qt::LeftButton) {
                index = popup->indexAt(mouseEvent->pos());
            }
        } else if (event->type() == QEvent::KeyPress) {
            const QKeyEvent *keyEvent = static_cast<QKeyEvent *>(event);

            const int keyInt = keyEvent->modifiers() | keyEvent->key();
            const QKeySequence eventSeq(keyInt);
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
            const int row = index.row();
            const int type = index.data(Qt::UserRole).toInt();

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
    flags.clear();
    const auto flags = Core()->getAllFlags();
    for (const auto &f : flags) {
        this->flags.append(f.name);
    }

    QCollator collator;
    collator.setNumericMode(true);
    collator.setCaseSensitivity(Qt::CaseInsensitive);
    std::sort(
            this->flags.begin(), this->flags.end(),
            [&collator](const QString &a, const QString &b) { return collator.compare(a, b) < 0; });

    handleSearch(this->text());
}

void Omnibar::clear()
{
    QLineEdit::clear();

    completerModel->clear();
    completer->popup()->hide();
}

void Omnibar::goToEntry()
{
    const QString str = this->text();
    if (!str.isEmpty()) {
        if (auto memoryWidget = main->getLastMemoryWidget()) {
            const RVA offset = Core()->math(str);
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
    searchedText = Text;
    matchCount = 0;

    if (!append) {
        completerModel->clear();
        lastIndex = 0;
    } else {
        // remove "show more" and "show all" rows (last two)
        completerModel->removeRows(completerModel->rowCount() - 2, 2);
    }

    if (Text.isEmpty()) {
        return;
    }

    bool hasMore = false;
    int itemsInModel = completerModel->rowCount();

    for (int i = 0; i < flags.size(); ++i) {
        const QString &flag = flags[i];
        if (flag.contains(Text, Qt::CaseInsensitive)) {
            const bool showAll = !Config()->getOmnibarLimitEntries();
            if ((itemsInModel < maxShownItems) || showAll) {
                if (!append || i > lastIndex) {
                    auto *item = new QStandardItem(flag);
                    item->setData(ItemType::Standard, Qt::UserRole);
                    completerModel->appendRow(item);
                    ++itemsInModel;
                    lastIndex = i;
                }
            } else {
                hasMore = true;
            }
            ++matchCount;
        }
    }

    if (hasMore) {
        auto addActionRow = [&](const QString &text, ItemType type) {
            auto *actionItem = new QStandardItem(text);
            actionItem->setData(type, Qt::UserRole);

            const QPalette palette = this->palette();
#if QT_VERSION >= QT_VERSION_CHECK(5, 12, 0)
            const QColor placeholderColor = palette.color(QPalette::PlaceholderText);
#else
            QColor placeholderColor = palette.color(QPalette::Text);
            placeholderColor.setAlpha(128);
#endif
            actionItem->setForeground(placeholderColor);

            QFont font = actionItem->font();
            font.setItalic(true);
            actionItem->setFont(font);

            completerModel->appendRow(actionItem);
        };

        addActionRow(tr("Show More"), ItemType::ShowMore);
        addActionRow(tr("Show All (%1 of %2)").arg(maxShownItems).arg(matchCount),
                     ItemType::ShowAll);
    }

    completer->setCompletionPrefix("");
    completer->complete();
}

void Omnibar::handleShowMore(int currentRow)
{
    if (maxShownItems >= matchCount) {
        return;
    }
    maxShownItems = std::min(maxShownItems + Config()->getOmnibarEntriesIncrement(), matchCount);
    handleSearch(searchedText, true);
    completer->popup()->setCurrentIndex(completerModel->index(currentRow, 0));
}

void Omnibar::handleShowAll(int currentRow)
{
    if (maxShownItems >= matchCount) {
        return;
    }
    maxShownItems = matchCount;
    handleSearch(searchedText, true);
    completer->popup()->setCurrentIndex(completerModel->index(currentRow, 0));
}
