#include "ShortcutOptionsWidget.h"
#include "ui_ShortcutOptionsWidget.h"
#include "shortcuts/ShortcutManager.h"

ShortcutOptionsWidget::ShortcutOptionsWidget(QWidget *parent)
    : QDialog(parent), ui(new Ui::ShortcutOptionsWidget)
{
    ui->setupUi(this);
    populateShortcutTree();
    setupUiElements();
}

void ShortcutOptionsWidget::setupUiElements()
{
    connect(ui->filterEdit, &QLineEdit::textEdited, this,
            &ShortcutOptionsWidget::filterShortcutTree);

    ui->shortcutTree->setHeaderLabels({ tr("Command"), tr("Shortcut"), tr("Description") });
    ui->shortcutTree->setColumnWidth(0, 400);
    ui->shortcutTree->setSortingEnabled(true);
    ui->shortcutTree->sortItems(0, Qt::AscendingOrder);
    ui->shortcutTree->expandAll();
}

void ShortcutOptionsWidget::populateShortcutTree()
{
    const QHash<QString, QString> categories = {
        { "General", tr("General") },       { "Disassembly", tr("Disassembly") },
        { "Decompiler", tr("Decompiler") }, { "Strings", tr("Strings") },
        { "Graph", tr("Graph") },           { "Breakpoint", tr("Breakpoint") },
        { "Console", tr("Console") },       { "Hex", tr("Hex") },
        { "Debug", tr("Debug") },           { "Functions", tr("Functions") },
        { "Omnibar", tr("Omnibar") },       { "Exports", tr("Exports") },
        { "Imports", tr("Imports") },       { "Overview", tr("Graph Overview") },
    };

    QHash<QString, QTreeWidgetItem *> prefixToItem;
    for (auto it = categories.cbegin(); it != categories.cend(); ++it) {
        prefixToItem[it.key()] = createCategoryItem(it.value());
    }

    const auto shortcuts = Shortcuts()->getAllShortcuts();
    for (auto it = shortcuts.cbegin(); it != shortcuts.cend(); ++it) {
        QString name = it.key();
        Shortcut s = it.value();
        QTreeWidgetItem *parent = prefixToItem["General"];

        for (auto it = prefixToItem.cbegin(); it != prefixToItem.cend(); ++it) {
            const QString &prefix = it.key() + ".";
            if (name.startsWith(prefix)) {
                name = name.mid(prefix.length());
                parent = it.value();
                break;
            }
        }

        QTreeWidgetItem *item = new QTreeWidgetItem(parent);
        item->setText(0, name);

        QStringList shortcutTexts;
        const QList<QKeySequence> &sequences = s.keySequences;
        for (const QKeySequence &seq : sequences) {
            shortcutTexts << seq.toString(QKeySequence::NativeText);
        }
        item->setText(1, shortcutTexts.join(", "));

        item->setText(2, QCoreApplication::translate(s.context, s.text));
    }
}

QTreeWidgetItem *ShortcutOptionsWidget::createCategoryItem(const QString &label)
{
    QTreeWidgetItem *item = new QTreeWidgetItem(ui->shortcutTree);
    QFont bold;
    bold.setBold(true);
    item->setText(0, label);
    item->setFont(0, bold);
    item->setFirstColumnSpanned(true);
    return item;
}

void ShortcutOptionsWidget::filterShortcutTree(const QString &input)
{
    const QString filter = input.trimmed().toLower();

    for (int i = 0; i < ui->shortcutTree->topLevelItemCount(); ++i) {
        QTreeWidgetItem *category = ui->shortcutTree->topLevelItem(i);
        bool categoryMatches = category->text(0).toLower().contains(filter);
        bool hasVisibleChildren = false;

        for (int j = 0; j < category->childCount(); ++j) {
            QTreeWidgetItem *child = category->child(j);
            bool match = false;

            if (categoryMatches) {
                match = true;
            } else {
                for (int col = 0; col < child->columnCount(); ++col) {
                    if (child->text(col).toLower().contains(filter)) {
                        match = true;
                        break;
                    }
                }
            }
            child->setHidden(!match);
            if (match)
                hasVisibleChildren = true;
        }
        category->setHidden(!(categoryMatches || hasVisibleChildren));
    }
}

ShortcutOptionsWidget::~ShortcutOptionsWidget() {}
