#include "ListDockWidget.h"
#include "ui_ListDockWidget.h"
#include "core/MainWindow.h"
#include "common/Helpers.h"
#include "menus/AddressableItemContextMenu.h"

#include <QMenu>
#include <QResizeEvent>
#include <QShortcut>

ListDockWidget::ListDockWidget(MainWindow *main, QAction *action, SearchBarPolicy searchBarPolicy) :
    CutterDockWidget(main, action),
    ui(new Ui::ListDockWidget),
    tree(new CutterTreeWidget(this)),
    searchBarPolicy(searchBarPolicy)
{
    ui->setupUi(this);

    // Add Status Bar footer
    tree->addStatusBar(ui->verticalLayout);

    if (searchBarPolicy != SearchBarPolicy::Hide) {
        // Ctrl-F to show/hide the filter entry
        QShortcut *searchShortcut = new QShortcut(QKeySequence::Find, this);
        connect(searchShortcut, &QShortcut::activated, ui->quickFilterView, &QuickFilterView::showFilter);
        searchShortcut->setContext(Qt::WidgetWithChildrenShortcut);

        // Esc to clear the filter entry
        QShortcut *clearShortcut = new QShortcut(QKeySequence(Qt::Key_Escape), this);
        connect(clearShortcut, &QShortcut::activated, [this]() {
            ui->quickFilterView->clearFilter();
            ui->treeView->setFocus();
        });
        clearShortcut->setContext(Qt::WidgetWithChildrenShortcut);
    }

    qhelpers::setVerticalScrollMode(ui->treeView);

    ui->treeView->setMainWindow(mainWindow);

    if (searchBarPolicy != SearchBarPolicy::ShowByDefault) {
        ui->quickFilterView->closeFilter();
    }
}

ListDockWidget::~ListDockWidget() {}

void ListDockWidget::showCount(bool show)
{
    tree->showStatusBar(show);
}

void ListDockWidget::setModels(AddressableFilterProxyModel *objectFilterProxyModel)
{
    this->objectFilterProxyModel = objectFilterProxyModel;


    ui->treeView->setModel(objectFilterProxyModel);


    connect(ui->quickFilterView, &QuickFilterView::filterTextChanged,
            objectFilterProxyModel, &QSortFilterProxyModel::setFilterWildcard);
    connect(ui->quickFilterView, &QuickFilterView::filterClosed, ui->treeView,
            static_cast<void(QWidget::*)()>(&QWidget::setFocus));


    connect(ui->quickFilterView, &QuickFilterView::filterTextChanged, this, [this] {
        tree->showItemsNumber(this->objectFilterProxyModel->rowCount());
    });
}
