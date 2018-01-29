
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QDebug>
#include <QThread>

#include "JupyterConnection.h"

JupyterConnection::JupyterConnection(QObject *parent) : QObject(parent)
{
    process = nullptr;
}

JupyterConnection::~JupyterConnection()
{
    cmdServer->stop();
    process->terminate();
    urlProcess->terminate();
}

const char *urlPy = "from notebook import notebookapp\n"
        "import json\n"
        "import time\n"
        "\n"
        "time.sleep(3)\n"
        "\n"
        "servers = [si for si in notebookapp.list_running_servers()]\n"
        "print(json.dumps(servers))";

void JupyterConnection::start()
{
    process = new QProcess(this);
    connect(process, &QProcess::readyReadStandardError, this, &JupyterConnection::readStandardError);
    connect(process, &QProcess::readyReadStandardOutput, this, &JupyterConnection::readStandardOutput);
    connect(process, &QProcess::errorOccurred, this, [](QProcess::ProcessError error){ qWarning() << "Jupyter error occurred:" << error; });
    process->start("jupyter", {"notebook", "--no-browser", "-y"});

    urlProcess = new QProcess(this);
    connect(urlProcess, &QProcess::readyReadStandardOutput, this, &JupyterConnection::readUrlStandardOutput);
    urlProcess->start("python3", {"-c", urlPy});

    QThread *cmdServerThread = new QThread(this);
    cmdServer = new CommandServer();
    cmdServer->moveToThread(cmdServerThread);
    connect(cmdServer, &CommandServer::error, this, [](QString err){ qWarning() << "CmdServer error:" << err; });
    connect(cmdServerThread, SIGNAL (started()), cmdServer, SLOT (process()));
    connect(cmdServer, SIGNAL (finished()), cmdServerThread, SLOT (quit()));
    connect(cmdServer, SIGNAL (finished()), cmdServer, SLOT (deleteLater()));
    connect(cmdServerThread, SIGNAL (finished()), cmdServerThread, SLOT (deleteLater()));
    cmdServerThread->start();
}

void JupyterConnection::readStandardError()
{
    auto data = process->readAllStandardError();
    printf("Jupyter stderr: %s", data.constData());
}

void JupyterConnection::readStandardOutput()
{
    auto data = process->readAllStandardOutput();
    printf("Jupyter stdout: %s", data.constData());
}

void JupyterConnection::readUrlStandardOutput()
{
    QJsonDocument doc = QJsonDocument::fromJson(urlProcess->readAllStandardOutput());

    for(QJsonValue value : doc.array())
    {
        QJsonObject serverObject = value.toObject();
        QString url = serverObject["url"].toString() + "?token=" + serverObject["token"].toString();
        emit urlReceived(url);
        break;
    }
}
