#ifndef JUPYTERCONNECTION_H
#define JUPYTERCONNECTION_H

#ifdef CUTTER_ENABLE_JUPYTER

#include <QProcess>
#include <QMap>

class NestedIPyKernel;

class JupyterConnection : public QObject
{
    Q_OBJECT

public:
    static JupyterConnection *getInstance();

    JupyterConnection(QObject *parent = nullptr);
    ~JupyterConnection();

    void start();
    QString getUrl();

    long startNestedIPyKernel(const QStringList &argv);
    NestedIPyKernel *getNestedIPyKernel(long id);
    QVariant pollNestedIPyKernel(long id);

signals:
    void urlReceived(const QString &url);
    void creationFailed();

private:
    QMap<long, NestedIPyKernel *> kernels;
    long nextKernelId = 1;

    bool notebookInstanceExists = false;
};


#define Jupyter() (JupyterConnection::getInstance())

#endif // CUTTER_ENABLE_JUPYTER

#endif // JUPYTERCONNECTION_H
