#ifndef FLAGDIALOG_H
#define FLAGDIALOG_H

#include "core/CutterCommon.h"

#include <QDialog>

#include <memory>

namespace Ui {
class FlagDialog;
}

/**
 * @brief Dialog for modifying flags at a specific offset
 */
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
    RVA flagOffset;
};

#endif // FLAGDIALOG_H
