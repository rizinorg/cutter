#pragma once

#include <QDialog>
#include <memory>
#include "CutterDescriptions.h"

namespace Ui {
class BreakpointsDialog;
}

class BreakpointsDialog : public QDialog
{
    Q_OBJECT

public:
    explicit BreakpointsDialog(bool editMode = false, QWidget *parent = nullptr);
    BreakpointsDialog(const BreakpointDescription &editableBreakpoint, QWidget *parent = nullptr);
    BreakpointsDialog(RVA address, QWidget *parent = nullptr);
    ~BreakpointsDialog();

    QString getBreakpoints();
    BreakpointDescription getDescription();

private slots:
    void on_buttonBox_accepted();
    void on_buttonBox_rejected();

private:
    std::unique_ptr<Ui::BreakpointsDialog> ui;
    bool editMode = false;

    bool eventFilter(QObject *obj, QEvent *event);
};
