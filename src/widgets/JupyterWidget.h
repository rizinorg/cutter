
#ifndef JUPYTERWIDGET_H
#define JUPYTERWIDGET_H

#ifdef CUTTER_ENABLE_JUPYTER

#include <memory>

#include <QDockWidget>

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

#ifdef CUTTER_ENABLE_QTWEBENGINE
    JupyterWebView *createNewTab();
#endif

private slots:
    void urlReceived(const QString &url);
    void creationFailed();

private:
    std::unique_ptr<Ui::JupyterWidget> ui;
};

#ifdef CUTTER_ENABLE_QTWEBENGINE

#include <QWebEngineView>

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

#endif

#endif

#endif //JUPYTERWIDGET_H
