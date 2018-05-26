
#include "AsyncTaskDialog.h"
#include "utils/AsyncTask.h"

#include "ui_AsyncTaskDialog.h"

AsyncTaskDialog::AsyncTaskDialog(AsyncTask *task, QWidget *parent)
    : QDialog(parent),
      ui(new Ui::AsyncTaskDialog),
      task(task)
{
    ui->setupUi(this);

    connect(task, &AsyncTask::logChanged, this, &AsyncTaskDialog::updateLog);

    updateLog();
}

AsyncTaskDialog::~AsyncTaskDialog()
{
}

void AsyncTaskDialog::updateLog()
{
    ui->logTextEdit->setPlainText(task->getLog());
}
