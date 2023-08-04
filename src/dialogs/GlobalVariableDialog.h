#ifndef GLOBALVARIABLEDIALOG_H
#define GLOBALVARIABLEDIALOG_H

#include <QDialog>
#include <memory>
#include "core/CutterCommon.h"

namespace Ui {
class GlobalVariableDialog;
}

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
