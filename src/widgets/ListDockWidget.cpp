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

    ui->quickFilterView->attachTo(ui->treeView, this);

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

    connect(ui->quickFilterView, &QuickFilterView::filterTextChanged, this, [this] {
        ui->quickFilterView->setItemCount(this->objectFilterProxyModel->rowCount());
    });
}
