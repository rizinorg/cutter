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
    static JupyterConnection* getInstance();

    JupyterConnection(QObject *parent = nullptr);
    ~JupyterConnection();

    void start();
    QString getUrl();

signals:
    void urlReceived(const QString &url);
    void creationFailed();

private:
    PyObject *cutterJupyterModule = nullptr;
    PyObject *cutterNotebookAppInstance = nullptr;

    PyThreadState *pyThreadState = nullptr;

    void initPython();
    void createCutterJupyterModule();
};


#define Jupyter() (JupyterConnection::getInstance())


#endif //JUPYTERCONNECTION_H
