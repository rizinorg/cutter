#ifndef JUPYTERCONNECTION_H
#define JUPYTERCONNECTION_H

#include <QProcess>

struct _object;
typedef _object PyObject;

struct _ts;
typedef _ts PyThreadState;

class JupyterConnection : public QObject
{
    Q_OBJECT

public:
    JupyterConnection(QObject *parent = nullptr);
    ~JupyterConnection();

    void start();

signals:
    void urlReceived(const QString &url);

private:
    PyObject *cutterJupyterModule = nullptr;
    PyObject *cutterNotebookAppInstance = nullptr;

    PyThreadState *pyThreadState = nullptr;
};

#endif //JUPYTERCONNECTION_H
