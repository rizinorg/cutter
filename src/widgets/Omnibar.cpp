#include "Omnibar.h"
#include "MainWindow.h"

#include <QStringListModel>
#include <QCompleter>
#include <QShortcut>
#include <QAbstractItemView>


Omnibar::Omnibar(MainWindow *main, QWidget *parent) :
    QLineEdit(parent),
    main(main)
{
    // QLineEdit basic features
    this->setMinimumHeight(16);
    this->setFrame(false);
    this->setPlaceholderText(tr("Type flag name or address here"));
    this->setStyleSheet("border-radius: 5px; padding: 0 8px; margin: 5px 0;");
    this->setTextMargins(10, 0, 0, 0);
    this->setClearButtonEnabled(true);

    connect(this, SIGNAL(returnPressed()), this, SLOT(on_gotoEntry_returnPressed()));

    // Esc clears omnibar
    QShortcut *clear_shortcut = new QShortcut(QKeySequence(Qt::Key_Escape), this);
    connect(clear_shortcut, SIGNAL(activated()), this, SLOT(clear()));
    clear_shortcut->setContext(Qt::WidgetWithChildrenShortcut);
}

void Omnibar::setupCompleter()
{
    // Set gotoEntry completer for jump history
    QCompleter *completer = new QCompleter(flags, this);
    completer->setMaxVisibleItems(20);
    completer->setCompletionMode(QCompleter::PopupCompletion);
    completer->setModelSorting(QCompleter::CaseSensitivelySortedModel);
    completer->setCaseSensitivity(Qt::CaseInsensitive);
    completer->setFilterMode(Qt::MatchContains);

    this->setCompleter(completer);
}

void Omnibar::refresh(const QStringList &flagList)
{
    flags = flagList;

    setupCompleter();
}

void Omnibar::restoreCompleter()
{
    QCompleter *completer = this->completer();
    completer->setFilterMode(Qt::MatchContains);
}

void Omnibar::clear()
{
    QLineEdit::clear();

    // Close the potential shown completer popup
    clearFocus();
    setFocus();
}

void Omnibar::on_gotoEntry_returnPressed()
{
    QString str = this->text();
    if (!str.isEmpty()) {
        Core()->seek(str);
    }

    this->setText("");
    this->clearFocus();
    this->restoreCompleter();
}
