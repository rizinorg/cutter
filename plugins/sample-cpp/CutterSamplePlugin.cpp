#include <QLabel>
#include <QHBoxLayout>
#include <QPushButton>
#include <QAction>

#include "CutterSamplePlugin.h"
#include "common/TempConfig.h"
#include "common/Configuration.h"
#include "MainWindow.h"

void CutterSamplePlugin::setupPlugin()
{
}

void CutterSamplePlugin::setupInterface(MainWindow *main)
{
    QAction *action = new QAction("Sample C++ Plugin", main);
    action->setCheckable(true);
    CutterSamplePluginWidget *widget = new CutterSamplePluginWidget(main, action);
    main->addPluginDockWidget(widget, action);
}

CutterSamplePluginWidget::CutterSamplePluginWidget(MainWindow *main, QAction *action) :
    CutterDockWidget(main, action)
{
    this->setObjectName("CutterSamplePluginWidget");
    this->setWindowTitle("Sample C++ Plugin");
    QWidget *content = new QWidget();
    this->setWidget(content);

    QVBoxLayout *layout = new QVBoxLayout(content);
    content->setLayout(layout);
    text = new QLabel(content);
    text->setFont(Config()->getFont());
    text->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
    layout->addWidget(text);

    QPushButton *button = new QPushButton(content);
    button->setText("Want a fortune?");
    button->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Maximum);
    button->setMaximumHeight(50);
    button->setMaximumWidth(200);
    layout->addWidget(button);
    layout->setAlignment(button, Qt::AlignHCenter);

    connect(Core(), &CutterCore::seekChanged, this, &CutterSamplePluginWidget::on_seekChanged);
    connect(button, &QPushButton::clicked, this, &CutterSamplePluginWidget::on_buttonClicked);
}

void CutterSamplePluginWidget::on_seekChanged(RVA addr)
{
    Q_UNUSED(addr);
    QString res;
    {
        TempConfig tempConfig;
        tempConfig.set("scr.color", 0);
        res = Core()->cmd("?E `pi 1`");
    }
    text->setText(res);
}

void CutterSamplePluginWidget::on_buttonClicked()
{
    QString fortune = Core()->cmd("fo").replace("\n", "");
    QString res = Core()->cmdRaw("?E " + fortune);
    text->setText(res);
}
