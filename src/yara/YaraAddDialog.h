#ifndef YARA_ADD_DIALOG_H
#define YARA_ADD_DIALOG_H

#include <QDialog>
#include <memory>
#include "core/CutterCommon.h"

namespace Ui {
class YaraAddDialog;
}

class YaraAddDialog : public QDialog
{
    Q_OBJECT

public:
    explicit YaraAddDialog(RVA offset, QWidget *parent = nullptr);
    ~YaraAddDialog();

private slots:
    void buttonBoxAccepted();
    void buttonBoxRejected();

private:
    std::unique_ptr<Ui::YaraAddDialog> ui;
    RVA flagOffset;
};

#endif // YARA_ADD_DIALOG_H
