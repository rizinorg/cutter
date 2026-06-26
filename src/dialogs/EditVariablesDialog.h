#ifndef EDITVARIABLESDIALOG_H
#define EDITVARIABLESDIALOG_H

#include "CutterDescriptions.h"

#include <QDialog>

#include <memory>

namespace Ui {
class EditVariablesDialog;
}

/**
 * @brief Dialog for editing local variables within a specific function
 */
class EditVariablesDialog : public QDialog
{
    Q_OBJECT

public:
    explicit EditVariablesDialog(RVA offset, const QString &initialVar = QString(),
                                 QWidget *parent = nullptr);
    ~EditVariablesDialog();

    bool empty() const;
private slots:
    void applyFields();
    void updateFields();

private:
    std::unique_ptr<Ui::EditVariablesDialog> ui;
    RVA functionAddress;
    QList<VariableDescription> variables;

    void populateTypesComboBox();
};

#endif // EDITVARIABLESDIALOG_H
