#ifdef CUTTER_ENABLE_JUPYTER

#include <Python.h>
#include <marshal.h>

#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QDebug>
#include <QThread>
#include <QFile>
#include <QCoreApplication>
#include <QDir>

#include "JupyterConnection.h"
#include "NestedIPyKernel.h"
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
    if (pyThreadState) {
        PyEval_RestoreThread(pyThreadState);

        if (cutterNotebookAppInstance) {
            auto stopFunc = PyObject_GetAttrString(cutterNotebookAppInstance, "stop");
            PyObject_CallObject(stopFunc, nullptr);
            Py_DECREF(cutterNotebookAppInstance);
        }

        Py_Finalize();
    }

    if (pythonHome) {
        PyMem_RawFree(pythonHome);
    }
}


void JupyterConnection::initPython()
{
#if defined(APPIMAGE) || defined(MACOS_PYTHON_FRAMEWORK_BUNDLED)
    if (customPythonHome.isNull()) {
        auto pythonHomeDir = QDir(QCoreApplication::applicationDirPath());
#   ifdef APPIMAGE
        // Executable is in appdir/bin
        pythonHomeDir.cdUp();
        qInfo() << "Setting PYTHONHOME =" << pythonHomeDir.absolutePath() << " for AppImage.";
#   else // MACOS_PYTHON_FRAMEWORK_BUNDLED
        // @executable_path/../Frameworks/Python.framework/Versions/Current
        pythonHomeDir.cd("../Frameworks/Python.framework/Versions/Current");
        qInfo() << "Setting PYTHONHOME =" << pythonHomeDir.absolutePath() <<
                " for macOS Application Bundle.";
#   endif
        customPythonHome = pythonHomeDir.absolutePath();
    }
#endif

    if (!customPythonHome.isNull()) {
        qInfo() << "PYTHONHOME =" << customPythonHome;
        pythonHome = Py_DecodeLocale(customPythonHome.toLocal8Bit().constData(), nullptr);
        Py_SetPythonHome(pythonHome);
    }

    PyImport_AppendInittab("cutter", &PyInit_api);
    PyImport_AppendInittab("cutter_internal", &PyInit_api_internal);
    Py_Initialize();
    PyEval_InitThreads();

    pyThreadState = PyEval_SaveThread();
}

void JupyterConnection::createCutterJupyterModule()
{
    if (pyThreadState) {
        PyEval_RestoreThread(pyThreadState);
    }

    QFile moduleFile(":/python/cutter_jupyter.pyc");
    bool isBytecode = moduleFile.exists();
    if (!isBytecode) {
        moduleFile.setFileName(":/python/cutter_jupyter.py");
    }
    moduleFile.open(QIODevice::ReadOnly);
    QByteArray moduleCode = moduleFile.readAll();
    moduleFile.close();

    PyObject *moduleCodeObject;
    if (isBytecode) {
        moduleCodeObject = PyMarshal_ReadObjectFromString(moduleCode.constData() + 12,
                                                          moduleCode.size() - 12);
    } else {
        moduleCodeObject = Py_CompileString(moduleCode.constData(), "cutter_jupyter.py",
                                            Py_file_input);
    }
    if (!moduleCodeObject) {
        PyErr_Print();
        qWarning() << "Could not compile cutter_jupyter.";
        emit creationFailed();
        pyThreadState = PyEval_SaveThread();
        return;
    }
    cutterJupyterModule = PyImport_ExecCodeModule("cutter_jupyter", moduleCodeObject);
    if (!cutterJupyterModule) {
        PyErr_Print();
        qWarning() << "Could not import cutter_jupyter.";
        emit creationFailed();
        pyThreadState = PyEval_SaveThread();
        return;
    }
    Py_DECREF(moduleCodeObject);

    pyThreadState = PyEval_SaveThread();
}

void JupyterConnection::start()
{
    if (cutterNotebookAppInstance) {
        return;
    }

    if (!Py_IsInitialized()) {
        initPython();
    }

    if (!cutterJupyterModule) {
        createCutterJupyterModule();

        if (!cutterJupyterModule) {
            return;
        }
    }

    PyEval_RestoreThread(pyThreadState);
    auto startFunc = PyObject_GetAttrString(cutterJupyterModule, "start_jupyter");
    cutterNotebookAppInstance = PyObject_CallObject(startFunc, nullptr);
    pyThreadState = PyEval_SaveThread();

    emit urlReceived(getUrl());
}

QString JupyterConnection::getUrl()
{
    if (!cutterNotebookAppInstance) {
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
