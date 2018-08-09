#include <QLabel>
#include <QHBoxLayout>
#include <QPushButton>

#include "CutterSamplePlugin.h"
#include "utils/TempConfig.h"
#include "utils/Configuration.h"

void CutterSamplePlugin::setupPlugin(CutterCore *core)
{
    this->core = core;
    this->name = "SamplePlugin";
    this->description = "Just a sample plugin.";
    this->version = "1.0";
    this->author = "xarkes";
}

CutterDockWidget* CutterSamplePlugin::setupInterface(MainWindow *main, QAction* actions)
{
    // Instantiate dock widget
    dockable = new CutterSamplePluginWidget(main, actions);
    return dockable;
}

CutterSamplePluginWidget::CutterSamplePluginWidget(MainWindow *main, QAction *action) :
    CutterDockWidget(main, action)
{
    this->setObjectName("CutterSamplePluginWidget");
    this->setWindowTitle("Sample Plugin");
    QWidget *content = new QWidget();
    this->setWidget(content);

    QVBoxLayout *layout = new QVBoxLayout(this);
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
    QString res = Core()->cmd("?E `fo`");
    text->setText(res);
}
