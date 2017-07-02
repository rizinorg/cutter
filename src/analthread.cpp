#include "analthread.h"
#include "iaitorcore.h"
#include <QDebug>

AnalThread::AnalThread(QWidget *parent) :
    QThread(parent),
    core(nullptr),
    level(2)
{
}

AnalThread::~AnalThread()
{
    if (isRunning())
    {
        quit();
        wait();
    }
}

void AnalThread::start(IaitoRCore *core, int level)
{
    this->core = core;
    this->level = level;

    QThread::start();
}

// run() will be called when a thread starts
void AnalThread::run()
{
    //qDebug() << "Anal level: " << this->level;
    core->analyze(this->level);
}
