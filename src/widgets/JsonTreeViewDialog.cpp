#include <Cutter.h>
#include <QString>
#include "JsonTreeViewDialog.h"
#include "ui_JsonTreeViewDialog.h"

#include <QStringList>
#include <QStringListModel>

#include <QMessageBox>

JsonTreeViewDialog::JsonTreeViewDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::JsonTreeViewDialog)
{
    ui->setupUi(this);    
    view    = new QTreeView(this);
    model   = new JsonModel;
    view->resize(parent->size());
}

JsonTreeViewDialog::~JsonTreeViewDialog()
{
    delete ui;
}

bool JsonTreeViewDialog::setJsonTreeView() 
{
    qjsonCertificatesDoc = Core()->cmdj("iCj");
    QString qstrCertificates(qjsonCertificatesDoc.toJson(QJsonDocument::Compact));
    if (QString::compare("{}",qstrCertificates)) 
    {
        std::string strCertificates = qstrCertificates.toUtf8().constData();
    	model->loadJson(QByteArray::fromStdString(strCertificates));
    	view->setModel(model);
    	return true;
    }
    return false;
}