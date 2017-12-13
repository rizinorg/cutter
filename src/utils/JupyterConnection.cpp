
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>

#include "JupyterConnection.h"

JupyterConnection::JupyterConnection(QObject *parent) : QObject(parent)
{
    process = nullptr;
}

JupyterConnection::~JupyterConnection()
{
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
    process->start("jupyter", {"notebook", "--no-browser", "-y"});

    urlProcess = new QProcess(this);
    connect(urlProcess, &QProcess::readyReadStandardOutput, this, &JupyterConnection::readUrlStandardOutput);
    urlProcess->start("python3", {"-c", urlPy});
}

void JupyterConnection::readStandardError()
{
    auto data = process->readAllStandardError();
    printf("jupyter stderr: %s\n", data.constData());
}

void JupyterConnection::readStandardOutput()
{
    auto data = process->readAllStandardOutput();
    printf("jupyter stdout: %s\n", data.constData());
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
