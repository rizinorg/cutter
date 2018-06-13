#include "CutterSamplePlugin.h"

void CutterSamplePlugin::setupPlugin(CutterCore *core)
{
    this->core = core;
    this->name = "SamplePlugin";
    this->description = "Just a sample plugin.";
    this->version = "1.0";
    this->author = "xarkes";
}

void CutterSamplePlugin::setupInterface(QWidget *parent)
{
    // NOP
}
