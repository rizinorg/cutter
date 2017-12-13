
#ifndef JUPYTERWIDGET_H
#define JUPYTERWIDGET_H

#include <memory>

#include <QDockWidget>
#include <QWebEngineView>

#include "utils/JupyterConnection.h"

namespace Ui
{
    class JupyterWidget;
}

class JupyterWebView;

class JupyterWidget : public QDockWidget
{
Q_OBJECT

public:
    JupyterWidget(QWidget *parent = nullptr, Qt::WindowFlags flags = Qt::WindowFlags());
    ~JupyterWidget();

    JupyterWebView *createNewTab();

private slots:
    void urlReceived(const QString &url);

private:
    std::unique_ptr<Ui::JupyterWidget> ui;

    JupyterConnection *jupyter;
};


class JupyterWebView : public QWebEngineView
{
Q_OBJECT

public:
    JupyterWebView(JupyterWidget *mainWidget, QWidget *parent = nullptr);

protected:
    QWebEngineView *createWindow(QWebEnginePage::WebWindowType type) override;

private:
    JupyterWidget *mainWidget;
};

#endif //JUPYTERWIDGET_H
