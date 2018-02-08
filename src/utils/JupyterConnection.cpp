
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
}

const char *jupyterPyCode = "import sys\n"
        "from notebook.notebookapp import *\n"
        "\n"
        "\n"
        "class CutterNotebookApp(NotebookApp):\n"
        "    def start(self):\n"
        "        \"\"\" see NotebookApp.start() \"\"\"\n"
        "\n"
        "        super(NotebookApp, self).start()\n"
        "\n"
        "        self.write_server_info_file()\n"
        "\n"
        "        if self.token and self._token_generated:\n"
        "            url = url_concat(self.connection_url, {'token': self.token})\n"
        "            sys.stdout.write(url + \"\\n\")\n"
        "            sys.stdout.flush()\n"
        "\n"
        "        self.io_loop = ioloop.IOLoop.current()\n"
        "        if sys.platform.startswith('win'):\n"
        "            # add no-op to wake every 5s\n"
        "            # to handle signals that may be ignored by the inner loop\n"
        "            pc = ioloop.PeriodicCallback(lambda: None, 5000)\n"
        "            pc.start()\n"
        "        try:\n"
        "            self.io_loop.start()\n"
        "        except KeyboardInterrupt:\n"
        "            self.log.info(_(\"Interrupted...\"))\n"
        "        finally:\n"
        "            self.remove_server_info_file()\n"
        "            self.cleanup_kernels()\n"
        "\n"
        "\n"
        "if __name__ == \"__main__\":\n"
        "    CutterNotebookApp.launch_instance()";

void JupyterConnection::start()
{
    process = new QProcess(this);
    connect(process, &QProcess::readyReadStandardError, this, &JupyterConnection::readStandardError);
    connect(process, &QProcess::readyReadStandardOutput, this, &JupyterConnection::readStandardOutput);
    connect(process, &QProcess::errorOccurred, this, [](QProcess::ProcessError error){ qWarning() << "Jupyter error occurred:" << error; });
    process->start("python3", {"-c", jupyterPyCode});

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
    printf("Jupyter stderr: %s\n", data.constData());
}

void JupyterConnection::readStandardOutput()
{
    auto data = process->readAllStandardOutput();
    printf("Jupyter stdout: %s\n", data.constData());
    emit urlReceived(data);
}