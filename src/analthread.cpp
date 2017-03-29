#include "analthread.h"
#include <QDebug>
#include "mainwindow.h"

AnalThread::AnalThread(MainWindow *main, QWidget *parent) :
    QThread(parent)
{
    // Radare core found in:
    this->main = main;
    //this->level = 2;
}

// run() will be called when a thread starts
void AnalThread::run()
{
    //qDebug() << "Anal level: " << this->level;
    this->main->core->analyze(this->level);
}
