#ifndef YARA_ADD_META_DIALOG_H
#define YARA_ADD_META_DIALOG_H

#include <QDialog>
#include <memory>
#include "core/CutterCommon.h"
#include "ui_YaraAddMetaDialog.h"

namespace Ui {
class YaraAddMetaDialog;
}

class YaraAddMetaDialog : public QDialog
{
    Q_OBJECT

public:
    static bool isKeyword(const QString &keyword);

    explicit YaraAddMetaDialog(QWidget *parent = nullptr);
    ~YaraAddMetaDialog();

public slots:
    void nameChanged(const QString &name);

private slots:
    void buttonBoxAccepted();
    void buttonBoxRejected();

private:
    std::unique_ptr<Ui::YaraAddMetaDialog> ui;

    static QStringList FileKeywords;
    static QStringList DateKeywords;
};

#endif // YARA_ADD_META_DIALOG_H
