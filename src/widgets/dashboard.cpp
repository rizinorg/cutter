#include "dashboard.h"
#include "ui_dashboard.h"

#include "mainwindow.h"

#include <QDebug>
#include <QJsonArray>
#include <QStringList>
#include <QJsonObject>
#include <QJsonDocument>
#include <QFile>
#include <QLayoutItem>


Dashboard::Dashboard(MainWindow *main, QWidget *parent) :
    DockWidget(parent),
    ui(new Ui::Dashboard)
{
    ui->setupUi(this);

    // Radare core found in:
    this->main = main;

    //this->updateContents();
}

Dashboard::~Dashboard()
{
    delete ui;
}

void Dashboard::setup()
{
    updateContents();
}

void Dashboard::refresh()
{
    updateContents();
}

void Dashboard::updateContents()
{
    // Parse and add JSON file info
    QString info = this->main->core->getFileInfo();

    QJsonDocument docu = QJsonDocument::fromJson(info.toUtf8());
    QJsonObject object = docu.object();
    QJsonValue value_core = object.value(QString("core"));
    QJsonObject item = value_core.toObject();
    //qDebug() << tr("QJsonObject of description: ") << item;

    QJsonValue value_bin = object.value(QString("bin"));
    //qDebug() << value;
    QJsonObject item2 = value_bin.toObject();
    //qDebug() << tr("QJsonObject of description: ") << item;

    this->ui->fileEdit->setText(item["file"].toString());
    this->ui->formatEdit->setText(item["format"].toString());
    this->ui->modeEdit->setText(item["mode"].toString());
    this->ui->typeEdit->setText(item["type"].toString());
    this->ui->sizeEdit->setText(QString::number(item["size"].toDouble()));
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

    if (item2["relro"].isUndefined()) {
        QString relro=item2["relro"].toString().split(" ").at(0);
        relro[0]=relro[0].toUpper();
        this->ui->relroEdit->setText(relro);
    }

    this->ui->baddrEdit->setText(QString::number(item2["baddr"].toDouble()));

    if (item2["va"].toBool() == true)
    {
        this->ui->vaEdit->setText("True");
    }
    else
    {
        this->ui->vaEdit->setText("False");
    }
    if (item2["canary"].toBool() == true)
    {
        this->ui->canaryEdit->setText("True");
    }
    else
    {
        this->ui->canaryEdit->setText("False");
        this->ui->canaryEdit->setStyleSheet("color: rgb(255, 0, 0);");
    }
    if (item2["crypto"].toBool() == true)
    {
        this->ui->cryptoEdit->setText("True");
    }
    else
    {
        this->ui->cryptoEdit->setText("False");
    }
    if (item2["nx"].toBool() == true)
    {
        this->ui->nxEdit->setText("True");
    }
    else
    {
        this->ui->nxEdit->setText("False");
        this->ui->nxEdit->setStyleSheet("color: rgb(255, 0, 0);");
    }
    if (item2["pic"].toBool() == true)
    {
        this->ui->picEdit->setText("True");
    }
    else
    {
        this->ui->picEdit->setText("False");
        this->ui->picEdit->setStyleSheet("color: rgb(255, 0, 0);");
    }
    if (item2["static"].toBool() == true)
    {
        this->ui->staticEdit->setText("True");
    }
    else
    {
        this->ui->staticEdit->setText("False");
    }
    if (item2["stripped"].toBool() == true)
    {
        this->ui->strippedEdit->setText("True");
    }
    else
    {
        this->ui->strippedEdit->setText("False");
    }
    if (item2["relocs"].toBool() == true)
    {
        this->ui->relocsEdit->setText("True");
    }
    else
    {
        this->ui->relocsEdit->setText("False");
    }

    // Add file hashes and libraries
    QString md5 = this->main->core->cmd("e file.md5");
    QString sha1 = this->main->core->cmd("e file.sha1");
    ui->md5Edit->setText(md5);
    ui->sha1Edit->setText(sha1);

    QString libs = this->main->core->cmd("il");
    QStringList lines = libs.split("\n", QString::SkipEmptyParts);
    if (!lines.isEmpty())
    {
        lines.removeFirst();
        lines.removeLast();
    }

    // dunno: why not label->setText(lines.join("\n")?
    while (ui->verticalLayout_2->count() > 0)
    {
        QLayoutItem *item = ui->verticalLayout_2->takeAt(0);
        if (item != nullptr)
        {
            QWidget *w = item->widget();
            if (w != nullptr)
            {
                w->deleteLater();
            }

            delete item;
        }
    }

    foreach (QString lib, lines)
    {
        QLabel *label = new QLabel(this);
        label->setText(lib);
        label->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
        ui->verticalLayout_2->addWidget(label);
    }




    QSpacerItem *spacer = new QSpacerItem(1, 1, QSizePolicy::Fixed, QSizePolicy::Expanding);
    ui->verticalLayout_2->addSpacerItem(spacer);

    // Add entropy value
    QString entropy = this->main->core->cmd("ph entropy").trimmed();
    ui->lblEntropy->setText(entropy);

    // Get stats for the graphs
    QStringList stats = this->main->core->getStats();

    // Add data to HTML graphs (stats)
    QFile html(":/html/stats.html");
    if (!html.open(QIODevice::ReadOnly))
    {
        QMessageBox::information(this, "error", html.errorString());
    }
    QString code = html.readAll();
    html.close();

    QString data = QString("%1, %2, %3, %4, %5, %6").arg(stats.at(0)).arg(stats.at(1)).arg(stats.at(2)).arg(stats.at(3)).arg(stats.at(4)).arg(stats.at(5));
    code.replace("MEOW", data);
    ui->statsWebView->setHtml(code);

    // Add data to polar graph
    QFile html2(":/html/radar.html");
    if (!html2.open(QIODevice::ReadOnly))
    {
        QMessageBox::information(this, "error", html2.errorString());
    }
    QString code2 = html2.readAll();
    html2.close();

    code2.replace("MEOW", data);
    code2.replace("WOEM", data);
    ui->polarWebView->setHtml(code2);
}
