#include <QLabel>
#include <QHBoxLayout>

#include "CutterSamplePlugin.h"

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
    dockable = new CutterDockWidget(main, actions);
    dockable->setWindowTitle("Sample Plugin");

    // Add a message
    QWidget *messageField = new QWidget(dockable);
    QLabel *label = new QLabel(messageField);
    label->setText("Hey, this plugin is just an example to show you how to create one.");
    QHBoxLayout *layout = new QHBoxLayout();
    layout->addWidget(label);
    layout->setAlignment(label, Qt::AlignCenter);
    messageField->setLayout(layout);

    return dockable;
}
