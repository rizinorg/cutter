#ifndef EDITFUNCTIONDIALOG_H
#define EDITFUNCTIONDIALOG_H

#include <QDialog>

#include <memory>

namespace Ui {
class EditFunctionDialog;
}

/**
 * @brief Dialog for editing functions
 */
class EditFunctionDialog : public QDialog
{
    Q_OBJECT

public:
    explicit EditFunctionDialog(QWidget *parent = nullptr);
    ~EditFunctionDialog();
    QString getNameText();
    void setNameText(const QString &name);
    QString getStartAddrText();
    void setStartAddrText(const QString &startAddr);
    QString getEndAddrText();
    void setEndAddrText(const QString &endAddr);
    QString getStackSizeText();
    void setStackSizeText(const QString &stackSize);
    void setCallConList(const QStringList &callConList);
    void setCallConSelected(const QString &selected);
    QString getCallConSelected();

private slots:
    void onButtonBoxAccepted();

    void onButtonBoxRejected();

private:
    std::unique_ptr<Ui::EditFunctionDialog> ui;
};

#endif // EDITFUNCTIONDIALOG_H
