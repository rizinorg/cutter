#include "Dashboard.h"
#include "ui_Dashboard.h"
#include "common/Helpers.h"
#include "common/JsonModel.h"
#include "common/JsonTreeItem.h"
#include "common/TempConfig.h"
#include "dialogs/VersionInfoDialog.h"

#include "MainWindow.h"
#include "CutterTreeView.h"

#include <QDebug>
#include <QJsonArray>
#include <QStringList>
#include <QJsonObject>
#include <QJsonDocument>
#include <QFile>
#include <QLayoutItem>
#include <QString>
#include <QMessageBox>
#include <QDialog>
#include <QTreeWidget>

Dashboard::Dashboard(MainWindow *main, QAction *action) :
    CutterDockWidget(main, action),
    ui(new Ui::Dashboard)
{
    ui->setupUi(this);

    connect(Core(), SIGNAL(refreshAll()), this, SLOT(updateContents()));
}

Dashboard::~Dashboard() {}

void Dashboard::updateContents()
{
    QJsonDocument docu = Core()->getFileInfo();
    QJsonObject item = docu.object()["core"].toObject();
    QJsonObject item2 = docu.object()["bin"].toObject();

    this->ui->fileEdit->setText(item["file"].toString());
    this->ui->formatEdit->setText(item["format"].toString());
    this->ui->modeEdit->setText(item["mode"].toString());
    this->ui->typeEdit->setText(item["type"].toString());
    this->ui->sizeEdit->setText(qhelpers::formatBytecount(item["size"].toDouble()));
    this->ui->fdEdit->setText(QString::number(item["fd"].toDouble()));

    this->ui->archEdit->setText(item2["arch"].toString());
    this->ui->langEdit->setText(item2["lang"].toString().toUpper());
    this->ui->classEdit->setText(item2["class"].toString());
    this->ui->machineEdit->setText(item2["machine"].toString());
    this->ui->osEdit->setText(item2["os"].toString());
    this->ui->subsysEdit->setText(item2["subsys"].toString());
    this->ui->endianEdit->setText(item2["endian"].toString());
    this->ui->compiledEdit->setText(item2["compiled"].toString());
    this->ui->bitsEdit->setText(QString::number(item2["bits"].toDouble()));

    if (!item2["relro"].isUndefined()) {
        QString relro = item2["relro"].toString().split(" ").at(0);
        relro[0] = relro[0].toUpper();
        this->ui->relroEdit->setText(relro);
    }

    this->ui->baddrEdit->setText(RAddressString(item2["baddr"].toVariant().toULongLong()));

    if (item2["va"].toBool() == true) {
        this->ui->vaEdit->setText("True");
    } else {
        this->ui->vaEdit->setText("False");
    }
    if (item2["canary"].toBool() == true) {
        this->ui->canaryEdit->setText("True");
    } else {
        this->ui->canaryEdit->setText("False");
    }
    if (item2["crypto"].toBool() == true) {
        this->ui->cryptoEdit->setText("True");
    } else {
        this->ui->cryptoEdit->setText("False");
    }
    if (item2["nx"].toBool() == true) {
        this->ui->nxEdit->setText("True");
    } else {
        this->ui->nxEdit->setText("False");
    }
    if (item2["pic"].toBool() == true) {
        this->ui->picEdit->setText("True");
    } else {
        this->ui->picEdit->setText("False");
    }
    if (item2["static"].toBool() == true) {
        this->ui->staticEdit->setText("True");
    } else {
        this->ui->staticEdit->setText("False");
    }
    if (item2["stripped"].toBool() == true) {
        this->ui->strippedEdit->setText("True");
    } else {
        this->ui->strippedEdit->setText("False");
    }
    if (item2["relocs"].toBool() == true) {
        this->ui->relocsEdit->setText("True");
    } else {
        this->ui->relocsEdit->setText("False");
    }

    // Add file hashes and libraries
    QString md5 = Core()->cmd("e file.md5");
    QString sha1 = Core()->cmd("e file.sha1");
    ui->md5Edit->setText(md5);
    ui->sha1Edit->setText(sha1);

    QString libs = Core()->cmd("il");
    QStringList lines = libs.split("\n", QString::SkipEmptyParts);
    if (!lines.isEmpty()) {
        lines.removeFirst();
        lines.removeLast();
    }

    // dunno: why not label->setText(lines.join("\n")?
    while (ui->verticalLayout_2->count() > 0) {
        QLayoutItem *item = ui->verticalLayout_2->takeAt(0);
        if (item != nullptr) {
            QWidget *w = item->widget();
            if (w != nullptr) {
                w->deleteLater();
            }

            delete item;
        }
    }

    for (const QString &lib : lines) {
        QLabel *label = new QLabel(this);
        label->setText(lib);
        label->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
        ui->verticalLayout_2->addWidget(label);
    }




    QSpacerItem *spacer = new QSpacerItem(1, 1, QSizePolicy::Fixed, QSizePolicy::Expanding);
    ui->verticalLayout_2->addSpacerItem(spacer);

    // Add entropy value
    {
        // Scope for TempConfig
        TempConfig tempConfig;
        tempConfig.set("io.va", false);
        QString entropy = Core()->cmd("ph entropy $s @ 0").trimmed();
        ui->lblEntropy->setText(entropy);
    }


    // Get stats for the graphs
    QStringList stats = Core()->getStats();

    // Check if signature info and version info available
    if (Core()->getSignatureInfo().isEmpty()) {
        ui->certificateButton->setEnabled(false);
    }
    if (Core()->getFileVersionInfo().isEmpty()) {
        ui->versioninfoButton->setEnabled(false);
    }

}

void Dashboard::on_certificateButton_clicked()
{
    static QDialog *viewDialog = nullptr;
    static CutterTreeView *view = nullptr;
    static JsonModel *model = nullptr;
    static QString qstrCertificates;
    if (!viewDialog) {
        viewDialog = new QDialog(this);
        view = new CutterTreeView(viewDialog);
        model = new JsonModel();
        QJsonDocument qjsonCertificatesDoc = Core()->getSignatureInfo();
        qstrCertificates = qjsonCertificatesDoc.toJson(QJsonDocument::Compact);
    }
    if (!viewDialog->isVisible()) {
        std::string strCertificates = qstrCertificates.toUtf8().constData();
        model->loadJson(QByteArray::fromStdString(strCertificates));
        view->setModel(model);
        view->expandAll();
        view->resize(900, 600);
        QSizePolicy sizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
        sizePolicy.setHorizontalStretch(0);
        sizePolicy.setVerticalStretch(0);
        sizePolicy.setHeightForWidth(view->sizePolicy().hasHeightForWidth());
        viewDialog->setSizePolicy(sizePolicy);
        viewDialog->setMinimumSize(QSize(900, 600));
        viewDialog->setMaximumSize(QSize(900, 600));
        viewDialog->setSizeGripEnabled(false);
        viewDialog->setWindowTitle("Certificates");
        viewDialog->show();
    }
}

void Dashboard::on_versioninfoButton_clicked()
{

    static QDialog *infoDialog = nullptr;

    if (!infoDialog) {
        infoDialog = new VersionInfoDialog(this);
    }

    if (!infoDialog->isVisible()) {
        infoDialog->show();
    }
}
