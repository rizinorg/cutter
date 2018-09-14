#ifndef SETFUNCTIONVARTYPES_H
#define SETFUNCTIONVARTYPES_H

#include "Cutter.h"
#include <QDialog>

namespace Ui {
class SetFunctionVarTypes;
}

class SetFunctionVarTypes : public QDialog
{
    Q_OBJECT

public:
    explicit SetFunctionVarTypes(QWidget *parent = nullptr);
    ~SetFunctionVarTypes();

    void setUserMessage(const QString userMessage);
    void setFcn(RAnalFunction *fcn);

public slots:
    void on_OkPressed();
    void on_ComboBoxChanged(QString varName);


private:
    RList                   *fcnVars;
    RAnalFunction           *fcn;
    Ui::SetFunctionVarTypes *ui;
    bool                    validFcn;
    void             populateTypesComboBox();

};

#endif // SETFUNCTIONVARTYPES_H
