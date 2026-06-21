#ifndef RZTASKDIALOG_H
#define RZTASKDIALOG_H

#include "common/RizinTask.h"
#include "core/CutterCommon.h"

#include <QDialog>
#include <QElapsedTimer>
#include <QTimer>

#include <memory>

class RizinTask;
namespace Ui {
class RizinTaskDialog;
}

/**
 * @brief A modal dialog that shows the progress of background @ref RizinTask
 */
class CUTTER_EXPORT RizinTaskDialog : public QDialog
{
    Q_OBJECT

public:
    ;
    RizinTaskDialog(const RizinTask::Ptr &task, QWidget *parent = nullptr);
    ~RizinTaskDialog();

    void setBreakOnClose(bool v) { breakOnClose = v; }
    bool getBreakOnClose() const { return breakOnClose; }
    void setDesc(const QString &label);

public slots:
    void reject() override;

private slots:
    void updateProgressTimer();

protected:
    void closeEvent(QCloseEvent *event) override;

private:
    std::unique_ptr<Ui::RizinTaskDialog> ui;
    std::shared_ptr<RizinTask> task;

    QTimer timer;
    QElapsedTimer elapsedTimer;

    bool breakOnClose = false;
};

#endif // RZTASKDIALOG_H
