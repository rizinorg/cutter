#include "CutterTreeView.h"
#include "ui_CutterTreeView.h"

CutterTreeView::CutterTreeView(QWidget *parent) : QTreeView(parent), ui(new Ui::CutterTreeView())
{
    ui->setupUi(this);
    applyCutterStyle(this);
}

CutterTreeView::~CutterTreeView() {}

void CutterTreeView::applyCutterStyle(QTreeView *view)
{
    view->setSelectionMode(QAbstractItemView::ExtendedSelection);
    view->setUniformRowHeights(true);
}
