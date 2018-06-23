#include "PythonManager.h"
#include "plugins/CutterPythonPlugin.h"
#include "PythonAPI.h"

#include <marshal.h>
#include <QDebug>
#include <QFile>

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
    QList<CutterPlugin *> plugins = Core()->getCutterPlugins();
    for (CutterPlugin *plugin : plugins) {
        delete plugin;
    }

    restoreThread();

    if (cutterNotebookAppInstance) {
        auto stopFunc = PyObject_GetAttrString(cutterNotebookAppInstance, "stop");
        PyObject_CallObject(stopFunc, nullptr);
        Py_DECREF(cutterNotebookAppInstance);
    }

    Py_Finalize();

    if (pythonHome) {
        PyMem_RawFree(pythonHome);
    }
}

void PythonManager::initialize()
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

    saveThread();

    // Import other modules
    cutterJupyterModule = createModule("cutter_jupyter");
    cutterPluginModule = createModule("cutter_plugin");
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

bool PythonManager::startJupyterNotebook()
{
    restoreThread();

    PyObject* startFunc = PyObject_GetAttrString(cutterJupyterModule, "start_jupyter");
    if (!startFunc) {
        qWarning() << "Couldn't get attribute start_jupyter.";
        return false;
    }

    cutterNotebookAppInstance = PyObject_CallObject(startFunc, nullptr);
    saveThread();

    return cutterNotebookAppInstance != nullptr;
}

QString PythonManager::getJupyterUrl()
{
    restoreThread();

    auto urlWithToken = PyObject_GetAttrString(cutterNotebookAppInstance, "url_with_token");
    auto asciiBytes = PyUnicode_AsASCIIString(urlWithToken);
    auto urlWithTokenString = QString::fromUtf8(PyBytes_AsString(asciiBytes));
    Py_DECREF(asciiBytes);
    Py_DECREF(urlWithToken);

    saveThread();

    return urlWithTokenString;
}

PyObject* PythonManager::createModule(QString module)
{
    PyObject *result = nullptr;

    restoreThread();
    QFile moduleFile(":/python/" + module + ".pyc");
    bool isBytecode = moduleFile.exists();
    if (!isBytecode) {
        moduleFile.setFileName(":/python/" + module + ".py");
    }
    moduleFile.open(QIODevice::ReadOnly);
    QByteArray moduleCode = moduleFile.readAll();
    moduleFile.close();

    PyObject *moduleCodeObject;
    if (isBytecode) {
        moduleCodeObject = PyMarshal_ReadObjectFromString(moduleCode.constData() + 12,
                                                          moduleCode.size() - 12);
    } else {
        moduleCodeObject = Py_CompileString(moduleCode.constData(), QString("%1.py").arg(module).toLatin1().constData(),
                                            Py_file_input);
    }
    if (!moduleCodeObject) {
        PyErr_Print();
        qWarning() << "Could not compile " + module + ".";
        pyThreadState = PyEval_SaveThread();
        return result;
    }
    result = PyImport_ExecCodeModule(module.toLatin1().constData(), moduleCodeObject);
    if (!result) {
        PyErr_Print();
        qWarning() << "Could not import " + module + ".";
        pyThreadState = PyEval_SaveThread();
        return result;
    }
    Py_DECREF(moduleCodeObject);

    saveThread();

    return result;
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
