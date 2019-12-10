#ifndef NATIVEDEBUGDIALOG_H
#define NATIVEDEBUGDIALOG_H

#include <QDialog>
#include <memory>

namespace Ui {
class NativeDebugDialog;
}

/**
 * @brief Dialog for connecting to native debug
 */
class NativeDebugDialog : public QDialog
{
    Q_OBJECT

public:
    explicit NativeDebugDialog(QWidget *parent = nullptr);
    ~NativeDebugDialog();

    QString getArgs() const;
    void setArgs(const QString &args);

private:
    std::unique_ptr<Ui::NativeDebugDialog> ui;
};

#endif // NATIVE_DEBUG_DIALOG
