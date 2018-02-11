
#include <Python.h>

#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QDebug>
#include <QThread>

#include "JupyterConnection.h"

JupyterConnection::JupyterConnection(QObject *parent) : QObject(parent)
{
}

JupyterConnection::~JupyterConnection()
{
    if (cutterNotebookAppInstance)
    {
        PyEval_RestoreThread(pyThreadState);

        auto stopFunc = PyObject_GetAttrString(cutterNotebookAppInstance, "stop");
        PyObject_CallObject(stopFunc, nullptr);
        Py_DECREF(cutterNotebookAppInstance);

        Py_FinalizeEx();
    }
}

void JupyterConnection::start()
{
    Py_Initialize();
    PyEval_InitThreads();

    cutterJupyterModule = PyImport_ImportModule("cutter_jupyter");
    if (!cutterJupyterModule)
    {
        qWarning() << "Could not import cutter_jupyter.";
        return;
    }
    auto startFunc = PyObject_GetAttrString(cutterJupyterModule, "start_jupyter");
    cutterNotebookAppInstance = PyObject_CallObject(startFunc, nullptr);
    auto urlWithToken = PyObject_GetAttrString(cutterNotebookAppInstance, "url_with_token");
    auto asciiBytes = PyUnicode_AsASCIIString(urlWithToken);
    emit urlReceived(QString::fromUtf8(PyBytes_AsString(asciiBytes)));
    Py_DECREF(asciiBytes);
    Py_DECREF(urlWithToken);

    pyThreadState = PyEval_SaveThread();
}
