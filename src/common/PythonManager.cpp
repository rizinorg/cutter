#include "PythonAPI.h"
#include "PythonManager.h"

#include <marshal.h>
#include <QFile>
#include <QDebug>

#include "QtResImporter.h"
#include "plugins/CutterPythonPlugin.h"

Q_GLOBAL_STATIC(PythonManager, uniqueInstance)

PythonManager *PythonManager::getInstance()
{
    return uniqueInstance;
}

PythonManager::PythonManager()
{
}

PythonManager::~PythonManager()
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

void PythonManager::initPythonHome()
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
}

void PythonManager::initialize()
{
    initPythonHome();

    PyImport_AppendInittab("_cutter", &PyInit_api);
    PyImport_AppendInittab("cutter_internal", &PyInit_api_internal);
    PyImport_AppendInittab("_qtres", &PyInit_qtres);
    Py_Initialize();
    PyEval_InitThreads();

    // Import other modules
    cutterJupyterModule = QtResImport("cutter_jupyter");
    cutterPluginModule = QtResImport("cutter_plugin");

    pyThreadState = PyEval_SaveThread();
}

void PythonManager::addPythonPath(char *path) {
    if (pyThreadState) {
        PyEval_RestoreThread(pyThreadState);
    }

    PyObject *sysModule = PyImport_ImportModule("sys");
    if (!sysModule) {
        return;
    }
    PyObject *pythonPath = PyObject_GetAttrString(sysModule, "path");
    if (!pythonPath) {
        return;
    }
    PyObject *append = PyObject_GetAttrString(pythonPath, "append");
    if (!append) {
        return;
    }
    PyEval_CallFunction(append, "(s)", path);

    pyThreadState = PyEval_SaveThread();
}

bool PythonManager::startJupyterNotebook()
{
    PyEval_RestoreThread(pyThreadState);

    PyObject* startFunc = PyObject_GetAttrString(cutterJupyterModule, "start_jupyter");
    if (!startFunc) {
        qWarning() << "Couldn't get attribute start_jupyter.";
        return false;
    }

    cutterNotebookAppInstance = PyObject_CallObject(startFunc, nullptr);
    pyThreadState = PyEval_SaveThread();

    return cutterNotebookAppInstance != nullptr;
}

QString PythonManager::getJupyterUrl()
{
    PyEval_RestoreThread(pyThreadState);

    auto urlWithToken = PyObject_GetAttrString(cutterNotebookAppInstance, "url_with_token");
    auto asciiBytes = PyUnicode_AsASCIIString(urlWithToken);
    auto urlWithTokenString = QString::fromUtf8(PyBytes_AsString(asciiBytes));
    Py_DECREF(asciiBytes);
    Py_DECREF(urlWithToken);

    pyThreadState = PyEval_SaveThread();

    return urlWithTokenString;
}

CutterPythonPlugin* PythonManager::loadPlugin(char *pluginName) {
    CutterPythonPlugin *plugin = nullptr;
    if (!cutterPluginModule) {
        return plugin;
    }

    if (pyThreadState) {
        PyEval_RestoreThread(pyThreadState);
    }

    PyObject *pluginModule = PyImport_ImportModule(pluginName);
    if (!pluginModule) {
        qWarning() << "Couldn't import the plugin" << QString(pluginName);
    }
    plugin = new CutterPythonPlugin(pluginModule);

    pyThreadState = PyEval_SaveThread();

    return plugin;
}

PyObject *PythonManager::getAttrStringSafe(PyObject *object, const char* attribute)
{
    PyObject *result = nullptr;
    if (pyThreadState) {
        PyEval_RestoreThread(pyThreadState);
    }

    result = PyObject_GetAttrString(object, attribute);

    pyThreadState = PyEval_SaveThread();

    return result;
}
