#ifdef CUTTER_ENABLE_JUPYTER

#include <Python.h>

#include "JupyterConnection.h"
#include "NestedIPyKernel.h"
#include "PythonManager.h"

#include <QVariant>
#include <QDebug>

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
}


void JupyterConnection::start()
{
    if (notebookInstanceExists) {
        return;
    }

    notebookInstanceExists = Python()->startJupyterNotebook();

    emit urlReceived(getUrl());
}

QString JupyterConnection::getUrl()
{
    if (!notebookInstanceExists) {
        return nullptr;
    }

    QString url = Python()->getJupyterUrl();
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

#endif
