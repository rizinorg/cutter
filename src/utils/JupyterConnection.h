
#ifndef JUPYTERCONNECTION_H
#define JUPYTERCONNECTION_H

#include <QProcess>

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
    QProcess *process;
    QProcess *urlProcess;

private slots:
    void readStandardError();
    void readStandardOutput();

    void readUrlStandardOutput();
};

#endif //JUPYTERCONNECTION_H
