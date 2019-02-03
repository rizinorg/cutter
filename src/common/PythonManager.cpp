#include "PythonAPI.h"
#include "PythonManager.h"

#include <marshal.h>
#include <QDebug>
#include <QFile>
#include <QDebug>

#include "QtResImporter.h"
#include "plugins/CutterPythonPlugin.h"

static PythonManager *uniqueInstance = nullptr;

PythonManager *PythonManager::getInstance()
{
    if (!uniqueInstance) {
        uniqueInstance = new PythonManager();
    }
    return uniqueInstance;
}

PythonManager::PythonManager()
{
}

PythonManager::~PythonManager()
{
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

extern "C" PyObject *PyInit_CutterBindings();

void PythonManager::initialize()
{
    initPythonHome();

    PyImport_AppendInittab("_cutter", &PyInit_api);
    PyImport_AppendInittab("cutter_internal", &PyInit_api_internal);
    PyImport_AppendInittab("_qtres", &PyInit_qtres);
    PyImport_AppendInittab("CutterBindings", &PyInit_CutterBindings);
    Py_Initialize();
    PyEval_InitThreads();

    RegQtResImporter();

    // Import other modules
    cutterPluginModule = QtResImport("cutter_plugin");

    saveThread();
}

void PythonManager::shutdown()
{
    emit willShutDown();

    restoreThread();

    Py_Finalize();

    if (pythonHome) {
        PyMem_RawFree(pythonHome);
    }
}

void PythonManager::addPythonPath(char *path) {
    restoreThread();

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

    saveThread();
}


CutterPythonPlugin* PythonManager::loadPlugin(const char *pluginName) {
    CutterPythonPlugin *plugin = nullptr;
    if (!cutterPluginModule) {
        return plugin;
    }

    restoreThread();
    PyObject *pluginModule = PyImport_ImportModule(pluginName);
    if (!pluginModule) {
        qWarning() << "Couldn't load the plugin" << QString(pluginName);
        PyErr_PrintEx(10);
    } else {
        plugin = new CutterPythonPlugin(pluginModule);
    }
    saveThread();

    return plugin;
}

void PythonManager::restoreThread()
{
    if (pyThreadState) {
        PyEval_RestoreThread(pyThreadState);
    }
}

void PythonManager::saveThread()
{
    pyThreadState = PyEval_SaveThread();
}
