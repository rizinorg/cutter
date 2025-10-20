#ifndef FLAGDIALOG_H
#define FLAGDIALOG_H

#include <QDialog>
#include <memory>
#include "core/CutterCommon.h"

namespace Ui {
class FlagDialog;
}

class FlagDialog : public QDialog
{
    Q_OBJECT

public:
    explicit FlagDialog(RVA offset, QWidget *parent = nullptr, QString flagNameHint = {});
    ~FlagDialog();

private slots:
    void buttonBoxAccepted();
    void buttonBoxRejected();

private:
    std::unique_ptr<Ui::FlagDialog> ui;
    RVA offset;
    QString flagName;
    ut64 flagOffset;
};

#endif // FLAGDIALOG_H
