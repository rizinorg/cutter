#include "CutterTreeView.h"
#include "ui_CutterTreeView.h"

CutterTreeView::CutterTreeView(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::CutterTreeView())
{
    ui->setupUi(this);
}
