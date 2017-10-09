#include "Omnibar.h"
#include "MainWindow.h"

#include <QStringListModel>
#include <QCompleter>
#include <QShortcut>
#include <QAbstractItemView>


Omnibar::Omnibar(MainWindow *main, QWidget *parent) :
    QLineEdit(parent),
    main(main),
    commands(
{
    ": Comments toggle",
    ": Dashboard toggle",
    ": Exports toggle",
    ": Flags toggle",
    ": Functions toggle",
    ": Imports toggle",
    ": Lock/Unlock interface",
    ": Notepad toggle",
    ": Refresh contents",
    ": Relocs toggle",
    ": Run Script",
    ": Sections toggle",
    ": Strings toggle",
    ": Symbols toggle",
    ": Tabs up/down",
    ": Theme switch",
    ": Web server start/stop"
})
{
    // QLineEdit basic features
    this->setMinimumHeight(16);
    this->setMaximumHeight(16);
    this->setFrame(false);
    this->setPlaceholderText("Type flag name or address here");
    this->setStyleSheet("border-radius: 5px;");
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
    QCompleter *completer = new QCompleter(flags + commands, this);
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

void Omnibar::showCommands()
{
    this->setFocus();
    this->setText(": ");

    QCompleter *completer = this->completer();
    completer->setCompletionMode(QCompleter::PopupCompletion);
    completer->setModelSorting(QCompleter::CaseSensitivelySortedModel);
    completer->setCaseSensitivity(Qt::CaseInsensitive);
    completer->setFilterMode(Qt::MatchStartsWith);

    completer->setMaxVisibleItems(20);

    completer->setCompletionPrefix(": ");
    completer->complete();
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
    if (str.length() > 0)
    {
        if (str.contains(": "))
        {
            if (str.contains("Lock"))
            {
                this->main->on_actionLock_triggered();
            }
            else if (str.contains("Functions"))
            {
                this->main->on_actionFunctions_triggered();
            }
            else if (str.contains("Flags"))
            {
                this->main->on_actionFlags_triggered();
            }
            else if (str.contains("Sections"))
            {
                this->main->on_actionSections_triggered();
            }
            else if (str.contains("Strings"))
            {
                this->main->on_actionStrings_triggered();
            }
            else if (str.contains("Imports"))
            {
                this->main->on_actionImports_triggered();
            }
            else if (str.contains("Exports"))
            {
                this->main->on_actionExports_triggered();
            }
            else if (str.contains("Symbols"))
            {
                this->main->on_actionSymbols_triggered();
            }
            else if (str.contains("Refresh"))
            {
                this->main->refreshVisibleDockWidgets();
            }
            else if (str.contains("Relocs"))
            {
                this->main->on_actionReloc_triggered();
            }
            else if (str.contains("Comments"))
            {
                this->main->on_actionComents_triggered();
            }
            else if (str.contains("Notepad"))
            {
                this->main->on_actionNotepad_triggered();
            }
            else if (str.contains("Dashboard"))
            {
                this->main->on_actionDashboard_triggered();
            }
            else if (str.contains("Theme"))
            {
                this->main->toggleSideBarTheme();
            }
            else if (str.contains("Script"))
            {
                this->main->on_actionRun_Script_triggered();
            }
            else if (str.contains("Tabs"))
            {
                this->main->on_actionTabs_triggered();
            }
        }
        else
        {
            //CutterCore::getInstance()->seek(CutterCore::getInstance()->cmd("?v " + this->text()), this->text());
            QString off = CutterCore::getInstance()->cmd("afo " + this->text());
            CutterCore::getInstance()->seek(off.trimmed().toInt());
        }
    }

    // check which tab is open? update all tabs? hex, graph?
    //refreshMem( this->gotoEntry->text() );
    this->setText("");
    this->clearFocus();
    this->restoreCompleter();
}
