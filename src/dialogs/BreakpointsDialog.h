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

    BreakpointDescription getDescription();

    static void createNewBreakpoint(RVA address = RVA_INVALID, QWidget *parent = nullptr);
    static void editBreakpoint(const BreakpointDescription& breakpoint, QWidget *parent = nullptr);
private:
    std::unique_ptr<Ui::BreakpointsDialog> ui;
    bool editMode = false;

    void refreshOkButton();
    void onTypeChanged();
    void configureCheckboxRestrictions();
    int getHwPermissions();
};
