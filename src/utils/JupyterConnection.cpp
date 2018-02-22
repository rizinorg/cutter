
#include <Python.h>

#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QDebug>
#include <QThread>
#include <QFile>

#include "JupyterConnection.h"
#include "PythonAPI.h"

Q_GLOBAL_STATIC(JupyterConnection, uniqueInstance)

JupyterConnection *JupyterConnection::getInstance()
{
    return uniqueInstance;
}

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
    }

    if (Py_IsInitialized())
    {
        Py_FinalizeEx();
    }
}


void JupyterConnection::initPython()
{
    PyImport_AppendInittab("cutter", &PyInit_api);
    PyImport_AppendInittab("cutter_internal", &PyInit_api_internal);
    Py_Initialize();
    PyEval_InitThreads();

    pyThreadState = PyEval_SaveThread();
}

void JupyterConnection::createCutterJupyterModule()
{
    PyEval_RestoreThread(pyThreadState);

    QFile moduleFile(":/python/cutter_jupyter.py");
    moduleFile.open(QIODevice::ReadOnly);
    QByteArray moduleCode = moduleFile.readAll();
    moduleFile.close();

    auto moduleCodeObject = Py_CompileString(moduleCode.constData(), "cutter_jupyter.py", Py_file_input);
    if (!moduleCodeObject)
    {
        qWarning() << "Could not compile cutter_jupyter.";
        emit creationFailed();
        pyThreadState = PyEval_SaveThread();
        return;
    }
    cutterJupyterModule = PyImport_ExecCodeModule("cutter_jupyter", moduleCodeObject);
    Py_DECREF(moduleCodeObject);
    if (!cutterJupyterModule)
    {
        qWarning() << "Could not import cutter_jupyter.";
        emit creationFailed();
        pyThreadState = PyEval_SaveThread();
        return;
    }

    pyThreadState = PyEval_SaveThread();
}

void JupyterConnection::start()
{
    if (cutterNotebookAppInstance)
    {
        return;
    }

    if (!Py_IsInitialized())
    {
        initPython();
    }

    if (!cutterJupyterModule)
    {
        createCutterJupyterModule();
    }

    PyEval_RestoreThread(pyThreadState);
    auto startFunc = PyObject_GetAttrString(cutterJupyterModule, "start_jupyter");
    cutterNotebookAppInstance = PyObject_CallObject(startFunc, nullptr);
    pyThreadState = PyEval_SaveThread();

    emit urlReceived(getUrl());
}

QString JupyterConnection::getUrl()
{
    if (!cutterNotebookAppInstance)
    {
        return nullptr;
    }

    PyEval_RestoreThread(pyThreadState);

    auto urlWithToken = PyObject_GetAttrString(cutterNotebookAppInstance, "url_with_token");
    auto asciiBytes = PyUnicode_AsASCIIString(urlWithToken);
    auto urlWithTokenString = QString::fromUtf8(PyBytes_AsString(asciiBytes));
    Py_DECREF(asciiBytes);
    Py_DECREF(urlWithToken);

    pyThreadState = PyEval_SaveThread();

    return urlWithTokenString;
}