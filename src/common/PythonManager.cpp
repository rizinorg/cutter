#ifdef CUTTER_ENABLE_PYTHON

#include <cassert>

#include "PythonAPI.h"
#include "PythonManager.h"
#include "Cutter.h"

#include <QDebug>
#include <QFile>
#include <QDebug>
#include <QCoreApplication>
#include <QDir>

#ifdef CUTTER_ENABLE_PYTHON_BINDINGS
#include <shiboken.h>
#include <pyside.h>
#include <signalmanager.h>
#endif

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

#ifdef CUTTER_ENABLE_PYTHON_BINDINGS
static void pySideDestructionVisitor(SbkObject* pyObj, void* data)
{
    void **realData = reinterpret_cast<void**>(data);
    auto pyQApp = reinterpret_cast<SbkObject*>(realData[0]);
    auto pyQObjectType = reinterpret_cast<PyTypeObject*>(realData[1]);

    if (pyObj == pyQApp || !PyObject_TypeCheck(pyObj, pyQObjectType)) {
        return;
    }
    if (!Shiboken::Object::hasOwnership(pyObj) || !Shiboken::Object::isValid(pyObj, false)) {
        return;
    }

    const char *reprStr = "";
    PyObject *repr = PyObject_Repr(reinterpret_cast<PyObject *>(pyObj));
    PyObject *reprBytes;
    if (repr) {
        reprBytes = PyUnicode_AsUTF8String(repr);
        reprStr = PyBytes_AsString(reprBytes);
    }
    qWarning() << "Warning: QObject from Python remaining (leaked from plugin?):" << reprStr;
    if (repr) {
        Py_DecRef(reprBytes);
        Py_DecRef(repr);
    }

    Shiboken::Object::setValidCpp(pyObj, false);
    Py_BEGIN_ALLOW_THREADS
    Shiboken::callCppDestructor<QObject>(Shiboken::Object::cppPointer(pyObj, pyQObjectType));
    Py_END_ALLOW_THREADS
};
#endif

void PythonManager::shutdown()
{
    emit willShutDown();

    restoreThread();

#ifdef CUTTER_ENABLE_PYTHON_BINDINGS
    // This is necessary to prevent a segfault when the CutterCore instance is deleted after the Shiboken::BindingManager
    Core()->setProperty("_PySideInvalidatePtr", QVariant());

    // see PySide::destroyQCoreApplication()
    PySide::SignalManager::instance().clear();
    Shiboken::BindingManager& bm = Shiboken::BindingManager::instance();
    SbkObject* pyQApp = bm.retrieveWrapper(QCoreApplication::instance());
    PyTypeObject* pyQObjectType = Shiboken::Conversions::getPythonTypeObject("QObject*");
    void* data[2] = {pyQApp, pyQObjectType};
    bm.visitAllPyObjects(&pySideDestructionVisitor, &data);

    PySide::runCleanupFunctions();
#endif

    if (pythonHome) {
        PyMem_Free(pythonHome);
    }

    Py_Finalize();
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
