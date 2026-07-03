#include "LineDiffWidget.h"

#include "ui_LineDiffWidget.h"

LineDiffWidget::LineDiffWidget(CutterDiff *cutterDiff,QWidget *parent) : QWidget(parent), ui(new Ui::LineDiffWidget), cutterDiff(cutterDiff)
{
    ui->setupUi(this);
    setData();
}

LineDiffWidget::~LineDiffWidget()
{
    delete ui;
}

void LineDiffWidget::setData(){
    ui->textEdit->setPlainText(cutterDiff->lineDiff("hello\nhow\nare\nyou","hellow\nhow\nis\nyou"));
}
