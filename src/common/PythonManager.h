#ifndef PYTHONMANAGER_H
#define PYTHONMANAGER_H

#include <QObject>

class CutterPythonPlugin;
typedef struct _ts PyThreadState;
typedef struct _object PyObject;

class PythonManager
{
public:
    static PythonManager *getInstance();

    PythonManager();
    ~PythonManager();

    void setPythonHome(const QString pythonHome)
    {
        customPythonHome = pythonHome;
    }

    void initPythonHome();
    void initialize();
    void addPythonPath(char *path);

    bool startJupyterNotebook();
    QString getJupyterUrl();

    PyObject *createModule(QString module);
    CutterPythonPlugin *loadPlugin(const char *pluginName);

    void restoreThread();
    void saveThread();

private:
    QString customPythonHome;
    wchar_t *pythonHome = nullptr;
    PyThreadState *pyThreadState = nullptr;

    PyObject *cutterJupyterModule;
    PyObject *cutterPluginModule;
    PyObject *cutterNotebookAppInstance = nullptr;
};

#define Python() (PythonManager::getInstance())

#endif // PYTHONMANAGER_H
