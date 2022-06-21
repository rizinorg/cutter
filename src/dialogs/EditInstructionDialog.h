#ifndef EDITINSTRUCTIONDIALOG_H
#define EDITINSTRUCTIONDIALOG_H

#include <QDialog>
#include <memory>

namespace Ui {
class EditInstructionDialog;
}

enum InstructionEditMode { EDIT_NONE, EDIT_BYTES, EDIT_TEXT };

class EditInstructionDialog : public QDialog
{
    Q_OBJECT

public:
    explicit EditInstructionDialog(InstructionEditMode isEditingBytes, QWidget *parent = nullptr);
    ~EditInstructionDialog();

    QString getInstruction() const;
    void setInstruction(const QString &instruction);
    bool needsNops() const;

private slots:
    void on_buttonBox_accepted();
    void on_buttonBox_rejected();

    void updatePreview(const QString &input);

private:
    std::unique_ptr<Ui::EditInstructionDialog> ui;
    // defines if the user is editing bytes or asm
    InstructionEditMode editMode;
};

#endif // EDITINSTRUCTIONDIALOG_H
