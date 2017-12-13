
#include "ui_JupyterWidget.h"

#include "JupyterWidget.h"

#include <QWebEngineSettings>

JupyterWidget::JupyterWidget(QWidget *parent, Qt::WindowFlags flags) :
    QDockWidget(parent, flags),
    ui(new Ui::JupyterWidget)
{
    ui->setupUi(this);

    jupyter = new JupyterConnection(this);
    connect(jupyter, &JupyterConnection::urlReceived, this, &JupyterWidget::urlReceived);
    jupyter->start();
}

JupyterWidget::~JupyterWidget()
{
}

JupyterWebView *JupyterWidget::createNewTab()
{
    auto webView = new JupyterWebView(this);
    ui->tabWidget->addTab(webView, "Tab");
    return webView;
}


void JupyterWidget::urlReceived(const QString &url)
{
    createNewTab()->load(QUrl(url));
}



JupyterWebView::JupyterWebView(JupyterWidget *mainWidget, QWidget *parent) : QWebEngineView(parent)
{
    this->mainWidget = mainWidget;
}

QWebEngineView *JupyterWebView::createWindow(QWebEnginePage::WebWindowType type)
{
    switch (type)
    {
        case QWebEnginePage::WebBrowserTab:
            return mainWidget->createNewTab();
        default:
            return nullptr;
    }
}
