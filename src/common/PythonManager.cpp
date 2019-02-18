#ifdef CUTTER_ENABLE_PYTHON

#include <cassert>

#include "PythonAPI.h"
#include "PythonManager.h"
#include "Cutter.h"

#include <marshal.h>
#include <QDebug>
#include <QFile>
#include <QDebug>
#include <QCoreApplication>
#include <QDir>

#include "QtResImporter.h"

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

#ifdef CUTTER_ENABLE_PYTHON_BINDINGS
extern "C" PyObject *PyInit_CutterBindings();
#endif

void PythonManager::initialize()
{
    initPythonHome();

    PyImport_AppendInittab("_cutter", &PyInit_api);
#ifdef CUTTER_ENABLE_JUPYTER
    PyImport_AppendInittab("cutter_internal", &PyInit_api_internal);
#endif
    PyImport_AppendInittab("_qtres", &PyInit_qtres);
#ifdef CUTTER_ENABLE_PYTHON_BINDINGS
    PyImport_AppendInittab("CutterBindings", &PyInit_CutterBindings);
#endif
    Py_Initialize();
    PyEval_InitThreads();
    pyThreadStateCounter = 1; // we have the thread now => 1

    RegQtResImporter();

    saveThread();
}

void PythonManager::shutdown()
{
    emit willShutDown();

    // This is necessary to prevent a segfault when the CutterCore instance is deleted after the Shiboken::BindingManager
    Core()->setProperty("_PySideInvalidatePtr", QVariant());

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

void PythonManager::restoreThread()
{
    pyThreadStateCounter++;
    if (pyThreadStateCounter == 1 && pyThreadState) {
        PyEval_RestoreThread(pyThreadState);
    }
}

void PythonManager::saveThread()
{
    pyThreadStateCounter--;
    assert(pyThreadStateCounter >= 0);
    if (pyThreadStateCounter == 0) {
        pyThreadState = PyEval_SaveThread();
    }
}

#endif