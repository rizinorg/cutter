#include "ListDockWidget.h"

#include "common/Helpers.h"
#include "core/MainWindow.h"
#include "shortcuts/ShortcutManager.h"
#include "ui_ListDockWidget.h"

#include <QMenu>
#include <QResizeEvent>
#include <QShortcut>

ListDockWidget::ListDockWidget(MainWindow *main)
    : CutterDockWidget(main), ui(new Ui::ListDockWidget)
{
    ui->setupUi(this);

    // Ctrl-F to show/hide the filter entry
    QShortcut *searchShortcut = Shortcuts()->makeQShortcut("General.showFilter", this);
    connect(searchShortcut, &QShortcut::activated, ui->quickFilterView,
            &QuickFilterView::showFilter);
    searchShortcut->setContext(Qt::WidgetWithChildrenShortcut);

    // Esc to clear the filter entry
    QShortcut *clearShortcut = Shortcuts()->makeQShortcut("General.clearFilter", this);
    connect(clearShortcut, &QShortcut::activated, [this]() {
        ui->quickFilterView->clearFilter();
        ui->treeView->setFocus();
    });
    clearShortcut->setContext(Qt::WidgetWithChildrenShortcut);

    qhelpers::setVerticalScrollMode(ui->treeView);

    ui->treeView->setMainWindow(mainWindow);
}

ListDockWidget::~ListDockWidget() {}

void ListDockWidget::setModels(AddressableFilterProxyModel *objectFilterProxyModel)
{
    this->objectFilterProxyModel = objectFilterProxyModel;

    ui->treeView->setModel(static_cast<AddressableItemModelI *>(objectFilterProxyModel));

    connect(ui->quickFilterView, &QuickFilterView::filterTextChanged, objectFilterProxyModel,
            &QSortFilterProxyModel::setFilterWildcard);
    connect(ui->quickFilterView, &QuickFilterView::filterClosed, ui->treeView,
            static_cast<void (QWidget::*)()>(&QWidget::setFocus));

    connect(ui->quickFilterView, &QuickFilterView::filterTextChanged, this, [this] {
        ui->quickFilterView->setItemCount(this->objectFilterProxyModel->rowCount());
    });
}
