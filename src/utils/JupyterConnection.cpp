
#include <Python.h>

#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QDebug>
#include <QThread>
#include <QFile>

#include "JupyterConnection.h"
#include "PythonAPI.h"

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
    PyImport_AppendInittab("cutter", &PyInit_api);
    PyImport_AppendInittab("cutter_internal", &PyInit_api_internal);
    Py_Initialize();
    PyEval_InitThreads();

    QFile moduleFile(":/python/cutter_jupyter.py");
    moduleFile.open(QIODevice::ReadOnly);
    QByteArray moduleCode = moduleFile.readAll();
    moduleFile.close();

    auto moduleCodeObject = Py_CompileString(moduleCode.constData(), "cutter_jupyter.py", Py_file_input);
    if (!moduleCodeObject)
    {
        qWarning() << "Could not compile cutter_jupyter.";
        return;
    }
    cutterJupyterModule = PyImport_ExecCodeModule("cutter_jupyter", moduleCodeObject);
    Py_DECREF(moduleCodeObject);
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
