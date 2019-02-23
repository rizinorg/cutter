#ifdef CUTTER_ENABLE_JUPYTER

#include "common/JupyterConnection.h"
#include "JupyterWidget.h"
#include "ui_JupyterWidget.h"

#include <QTabWidget>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>

#ifdef CUTTER_ENABLE_QTWEBENGINE
#include <QWebEngineSettings>
#endif

JupyterWidget::JupyterWidget(MainWindow *main, QAction *action) :
    CutterDockWidget(main, action),
    ui(new Ui::JupyterWidget)
{
    ui->setupUi(this);

    ui->tabWidget->setTabsClosable(true);
    ui->tabWidget->setMovable(true);

    QWidget *cornerWidget = new QWidget(ui->tabWidget);
    QHBoxLayout *cornerWidgetLayout = new QHBoxLayout(cornerWidget);
    cornerWidget->setLayout(cornerWidgetLayout);
    cornerWidgetLayout->setContentsMargins(4, 4, 4, 4);
    homeButton = new QPushButton(cornerWidget);
    homeButton->setStyleSheet("QPushButton { padding: 2px; background-color: palette(light); border-radius: 4px; }"
                              "QPushButton:pressed { background-color: palette(dark); }");
    homeButton->setIcon(QIcon(":/img/icons/home.svg"));
    homeButton->setEnabled(false);
    cornerWidgetLayout->addWidget(homeButton);
    ui->tabWidget->setCornerWidget(cornerWidget);

    connect(homeButton, &QAbstractButton::clicked, this, &JupyterWidget::openHomeTab);
    connect(ui->tabWidget, &QTabWidget::tabCloseRequested, this, &JupyterWidget::tabCloseRequested);

    connect(Jupyter(), &JupyterConnection::urlReceived, this, &JupyterWidget::urlReceived);
    connect(Jupyter(), &JupyterConnection::creationFailed, this, &JupyterWidget::creationFailed);
    Jupyter()->start();
}

JupyterWidget::~JupyterWidget()
{
}

#ifdef CUTTER_ENABLE_QTWEBENGINE
JupyterWebView *JupyterWidget::createNewTab()
{
    auto webView = new JupyterWebView(this);
    int index = ui->tabWidget->addTab(webView, "Tab");
    webView->setTabWidget(ui->tabWidget);
    ui->tabWidget->setCurrentIndex(index);
    return webView;
}
#endif

void JupyterWidget::urlReceived(const QString &url)
{
#ifdef CUTTER_ENABLE_QTWEBENGINE
    Q_UNUSED(url);
    openHomeTab();
    homeButton->setEnabled(true);
#else
    clearTabs();
    QWidget *failPage = new QWidget(this);
    QLabel *label = new QLabel(failPage);
    label->setText(
        tr("Cutter has been built without QtWebEngine.<br />Open the following URL in your Browser to use Jupyter:<br /><a href=\"%1\">%1</a>").arg(
            url));
    label->setTextFormat(Qt::RichText);
    label->setTextInteractionFlags(Qt::TextBrowserInteraction);
    label->setOpenExternalLinks(true);
    QHBoxLayout *layout = new QHBoxLayout();
    layout->addWidget(label);
    layout->setAlignment(label, Qt::AlignCenter);
    failPage->setLayout(layout);
    ui->tabWidget->addTab(failPage, tr("Jupyter"));
    homeButton->setEnabled(false);
    ui->tabWidget->setTabsClosable(false);
#endif
}

void JupyterWidget::creationFailed()
{
    clearTabs();
    QWidget *failPage = new QWidget(this);
    QLabel *label = new QLabel(failPage);
    label->setText(
        tr("An error occurred while opening jupyter. Make sure Jupyter is installed system-wide."));
    QHBoxLayout *layout = new QHBoxLayout();
    layout->addWidget(label);
    layout->setAlignment(label, Qt::AlignCenter);
    failPage->setLayout(layout);
    ui->tabWidget->addTab(failPage, tr("Error"));
    homeButton->setEnabled(false);
    ui->tabWidget->setTabsClosable(false);
}

void JupyterWidget::openHomeTab()
{
#ifdef CUTTER_ENABLE_QTWEBENGINE
    QString url = Jupyter()->getUrl();
    if (!url.isNull()) {
        createNewTab()->load(QUrl(url));
    }
#endif
}

void JupyterWidget::tabCloseRequested(int index)
{
    removeTab(index);
    if (ui->tabWidget->count() == 0) {
        openHomeTab();
    }
}

void JupyterWidget::removeTab(int index)
{
    QWidget *widget = ui->tabWidget->widget(index);
    ui->tabWidget->removeTab(index);
    delete widget;
}

void JupyterWidget::clearTabs()
{
    while (ui->tabWidget->count() > 0) {
        removeTab(0);
    }
}

#ifdef CUTTER_ENABLE_QTWEBENGINE
JupyterWebView::JupyterWebView(JupyterWidget *mainWidget, QWidget *parent) : QWebEngineView(parent)
{
    this->mainWidget = mainWidget;
    this->tabWidget = nullptr;

    connect(this, &QWebEngineView::titleChanged, this, &JupyterWebView::onTitleChanged);
}

void JupyterWebView::setTabWidget(QTabWidget *tabWidget)
{
    this->tabWidget = tabWidget;
    updateTitle();
}

QWebEngineView *JupyterWebView::createWindow(QWebEnginePage::WebWindowType type)
{
    switch (type) {
    case QWebEnginePage::WebBrowserTab:
        return mainWidget->createNewTab();
    default:
        return nullptr;
    }
}

void JupyterWebView::onTitleChanged(const QString &)
{
    updateTitle();
}

void JupyterWebView::updateTitle()
{
    if (!tabWidget) {
        return;
    }

    QString title = this->title();
    if (title.isEmpty()) {
        title = tr("Jupyter");
    }
    tabWidget->setTabText(tabWidget->indexOf(this), title);
}

#endif

#endif
