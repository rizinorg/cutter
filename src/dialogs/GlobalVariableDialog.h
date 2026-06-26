#ifndef GLOBALVARIABLEDIALOG_H
#define GLOBALVARIABLEDIALOG_H

#include "core/CutterCommon.h"

#include <QDialog>

#include <memory>

namespace Ui {
class GlobalVariableDialog;
}

/**
 * @brief Dialog for defining, modifying, or deleting global variables at a specific address
 */
class GlobalVariableDialog : public QDialog
{
    Q_OBJECT

public:
    explicit GlobalVariableDialog(RVA offset, QWidget *parent = nullptr);
    ~GlobalVariableDialog();

private slots:
    void buttonBoxAccepted();
    void buttonBoxRejected();

private:
    std::unique_ptr<Ui::GlobalVariableDialog> ui;
    RVA offset;
    QString globalVariableName;
    RVA globalVariableOffset;
    QString typ;
};

#endif // GLOBALVARIABLEDIALOG_H
