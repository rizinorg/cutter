#include "CutterTreeView.h"
#include "ui_CutterTreeView.h"

CutterTreeView::CutterTreeView(QWidget *parent) :
    QTreeView(parent),
    ui(new Ui::CutterTreeView())
{
    ui->setupUi(this);
    this->setSelectionMode(QAbstractItemView::ExtendedSelection);
}

CutterTreeView::~CutterTreeView() {}
