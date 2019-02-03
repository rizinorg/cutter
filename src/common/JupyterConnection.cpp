#ifdef CUTTER_ENABLE_JUPYTER

#include <Python.h>

#include "JupyterConnection.h"
#include "NestedIPyKernel.h"
#include "PythonManager.h"
#include "QtResImporter.h"

#include <QVariant>
#include <QDebug>

Q_GLOBAL_STATIC(JupyterConnection, uniqueInstance)

JupyterConnection *JupyterConnection::getInstance()
{
    return uniqueInstance;
}

JupyterConnection::JupyterConnection(QObject *parent) : QObject(parent)
{
    connect(Python(), &PythonManager::willShutDown, this, &JupyterConnection::stop);
}

JupyterConnection::~JupyterConnection()
{
}


void JupyterConnection::start()
{
    if (notebookInstanceExists) {
        return;
    }

    notebookInstanceExists = startJupyterNotebook();

    emit urlReceived(getUrl());
}

void JupyterConnection::stop()
{
    if (cutterNotebookAppInstance) {
        Python()->restoreThread();
        auto stopFunc = PyObject_GetAttrString(cutterNotebookAppInstance, "stop");
        PyObject_CallObject(stopFunc, nullptr);
        Py_DECREF(cutterNotebookAppInstance);
        Python()->saveThread();
    }
}

QString JupyterConnection::getUrl()
{
    if (!notebookInstanceExists) {
        return nullptr;
    }

    QString url = getJupyterUrl();
    return url;
}

long JupyterConnection::startNestedIPyKernel(const QStringList &argv)
{
    NestedIPyKernel *kernel = NestedIPyKernel::start(argv);

    if (!kernel) {
        qWarning() << "Could not start nested IPyKernel.";
        return 0;
    }

    long id = nextKernelId++;
    kernels.insert(id, kernel);

    return id;
}

NestedIPyKernel *JupyterConnection::getNestedIPyKernel(long id)
{
    auto it = kernels.find(id);
    if (it == kernels.end()) {
        return nullptr;
    }
    return *it;
}

QVariant JupyterConnection::pollNestedIPyKernel(long id)
{
    auto it = kernels.find(id);
    if (it == kernels.end()) {
        return QVariant(0);
    }

    NestedIPyKernel *kernel = *it;
    QVariant v = kernel->poll();

    if (!v.isNull()) {
        // if poll of kernel returns anything but None, it has already quit and should be cleaned up
        PyThreadState *subinterpreterState = kernel->getThreadState();
        delete kernel;
        kernels.erase(it);

        PyThreadState *parentThreadState = PyThreadState_Swap(subinterpreterState);
        Py_EndInterpreter(subinterpreterState);
        PyThreadState_Swap(parentThreadState);
    }

    return v;
}

bool JupyterConnection::startJupyterNotebook()
{
    Python()->restoreThread();

    if (!cutterJupyterModule) {
        cutterJupyterModule = QtResImport("cutter_jupyter");
    }

    PyObject* startFunc = PyObject_GetAttrString(cutterJupyterModule, "start_jupyter");
    if (!startFunc) {
        qWarning() << "Couldn't get attribute start_jupyter.";
        return false;
    }

    cutterNotebookAppInstance = PyObject_CallObject(startFunc, nullptr);
    Python()->saveThread();

    return cutterNotebookAppInstance != nullptr;
}

QString JupyterConnection::getJupyterUrl()
{
    Python()->restoreThread();

    auto urlWithToken = PyObject_GetAttrString(cutterNotebookAppInstance, "url_with_token");
    auto asciiBytes = PyUnicode_AsASCIIString(urlWithToken);
    auto urlWithTokenString = QString::fromUtf8(PyBytes_AsString(asciiBytes));
    Py_DECREF(asciiBytes);
    Py_DECREF(urlWithToken);

    Python()->saveThread();

    return urlWithTokenString;
}

#endif
