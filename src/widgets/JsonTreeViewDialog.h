#ifndef JSONTREEVIEWDIALOG_H
#define JSONTREEVIEWDIALOG_H

#include <QDialog>
#include "utils/JsonModel.h"
#include "utils/JsonTreeItem.h"
#include <QTreeView>
#include <QJsonDocument>

namespace Ui {
class JsonTreeViewDialog;
}

class JsonTreeViewDialog : public QDialog {
        Q_OBJECT
public:
    explicit JsonTreeViewDialog(QWidget *parent = 0);
    ~JsonTreeViewDialog();
    QTreeView *view;
    JsonModel *model;
    QJsonDocument qjsonCertificatesDoc;
    bool setJsonTreeView();

private:
    Ui::JsonTreeViewDialog *ui;
};

#endif // JSONTREEVIEWDIALOG_H

