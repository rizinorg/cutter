#pragma once

#include <QDialog>
#include <memory>

namespace Ui {
class BreakpointsDialog;
}

class BreakpointsDialog : public QDialog
{
    Q_OBJECT

public:
    explicit BreakpointsDialog(QWidget *parent = nullptr);
    ~BreakpointsDialog();

    QString getBreakpoints();

private slots:
    void on_buttonBox_accepted();
    void on_buttonBox_rejected();

private:
    std::unique_ptr<Ui::BreakpointsDialog> ui;

    bool eventFilter(QObject *obj, QEvent *event);
};
