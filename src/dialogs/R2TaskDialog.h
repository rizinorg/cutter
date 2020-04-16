#ifndef R2TASKDIALOG_H
#define R2TASKDIALOG_H

#include <memory>

#include <QDialog>
#include <QTimer>
#include <QElapsedTimer>

#include "common/R2Task.h"

class R2Task;
namespace Ui {
class R2TaskDialog;
}

class R2TaskDialog : public QDialog
{
    Q_OBJECT

public:
    using Ptr = QSharedPointer<R2Task>;
    R2TaskDialog(Ptr task, QWidget *parent = nullptr);
    ~R2TaskDialog();

    void setBreakOnClose(bool v)        { breakOnClose = v; }
    bool getBreakOnClose()              { return breakOnClose; }
    void setDesc(const QString &label);

public slots:
    void reject() override;

private slots:
    void updateProgressTimer();

protected:
    void closeEvent(QCloseEvent *event) override;

private:
    std::unique_ptr<Ui::R2TaskDialog> ui;
    QSharedPointer<R2Task> task;

    QTimer timer;
    QElapsedTimer elapsedTimer;

    bool breakOnClose = false;
};

#endif //R2TASKDIALOG_H
