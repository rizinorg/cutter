#include "PythonManager.h"
#include "PythonAPI.h"

#include <marshal.h>
#include <QFile>

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

    pyThreadState = PyEval_SaveThread();

    // Import other modules
    cutterJupyterModule = createModule("cutter_jupyter");
    cutterPluginModule = createModule("cutter_plugin");
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

PyObject* PythonManager::createModule(QString module)
{
    PyObject *result = nullptr;
    if (pyThreadState) {
        PyEval_RestoreThread(pyThreadState);
    }

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

    pyThreadState = PyEval_SaveThread();

    return result;
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
