#ifndef PYTHONMANAGER_H
#define PYTHONMANAGER_H

#ifdef CUTTER_ENABLE_PYTHON

#include <QObject>

typedef struct _ts PyThreadState;
typedef struct _object PyObject;

class PythonManager: public QObject
{
    Q_OBJECT

public:
    static PythonManager *getInstance();

    PythonManager();
    ~PythonManager();

    void setPythonHome(const QString &pythonHome) { customPythonHome = pythonHome; }

    void initPythonHome();
    void initialize();
    void shutdown();

    void addPythonPath(char *path);

    void restoreThread();
    void saveThread();

    /*!
     * \brief RAII Helper class to call restoreThread() and saveThread() automatically
     *
     * As long as an object of this class is in scope, the Python thread will remain restored.
     */
    class ThreadHolder
    {
    public:
        ThreadHolder()    { getInstance()->restoreThread(); }
        ~ThreadHolder()   { getInstance()->saveThread(); }
    };

signals:
    void willShutDown();

private:
    QString customPythonHome;
    wchar_t *pythonHome = nullptr;
    PyThreadState *pyThreadState = nullptr;
    int pyThreadStateCounter = 0;
};

#define Python() (PythonManager::getInstance())

#endif // CUTTER_ENABLE_PYTHON

#endif // PYTHONMANAGER_H
