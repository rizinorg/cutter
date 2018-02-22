
#include "ui_JupyterWidget.h"

#include "JupyterWidget.h"

#include <QWebEngineSettings>
#include <QTabWidget>
#include <QHBoxLayout>
#include <QLabel>

JupyterWidget::JupyterWidget(QWidget *parent, Qt::WindowFlags flags) :
    QDockWidget(parent, flags),
    ui(new Ui::JupyterWidget)
{
    ui->setupUi(this);

    connect(Jupyter(), &JupyterConnection::urlReceived, this, &JupyterWidget::urlReceived);
    connect(Jupyter(), &JupyterConnection::creationFailed, this, &JupyterWidget::creationFailed);
    Jupyter()->start();
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

void JupyterWidget::creationFailed()
{
    QWidget *failPage = new QWidget(this);
    QLabel *label = new QLabel(failPage);
    label->setText(tr("An error occurred while opening jupyter. Make sure Jupyter is installed system-wide."));
    QHBoxLayout *layout = new QHBoxLayout();
    layout->addWidget(label);
    layout->setAlignment(label, Qt::AlignCenter);
    failPage->setLayout(layout);
    ui->tabWidget->addTab(failPage, tr("Error"));
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
