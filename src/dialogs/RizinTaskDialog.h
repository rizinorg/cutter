#ifndef RZTASKDIALOG_H
#define RZTASKDIALOG_H

#include <memory>

#include <QDialog>
#include <QTimer>
#include <QElapsedTimer>

#include "common/RizinTask.h"
#include "core/CutterCommon.h"

class RizinTask;
namespace Ui {
class RizinTaskDialog;
}

class CUTTER_EXPORT RizinTaskDialog : public QDialog
{
    Q_OBJECT

public:
    using Ptr = QSharedPointer<RizinTask>;
    RizinTaskDialog(Ptr task, QWidget *parent = nullptr);
    ~RizinTaskDialog();

    void setBreakOnClose(bool v) { breakOnClose = v; }
    bool getBreakOnClose() { return breakOnClose; }
    void setDesc(const QString &label);

public slots:
    void reject() override;

private slots:
    void updateProgressTimer();

protected:
    void closeEvent(QCloseEvent *event) override;

private:
    std::unique_ptr<Ui::RizinTaskDialog> ui;
    QSharedPointer<RizinTask> task;

    QTimer timer;
    QElapsedTimer elapsedTimer;

    bool breakOnClose = false;
};

#endif // RZTASKDIALOG_H
