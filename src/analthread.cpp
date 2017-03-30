#include "analthread.h"
#include <QDebug>
#include "mainwindow.h"

AnalThread::AnalThread(MainWindow *w, QWidget *parent) :
    QThread(parent)
{
    // Radare core found in:
    this->w = w;
    //this->level = 2;
}

// run() will be called when a thread starts
void AnalThread::run()
{
    //qDebug() << "Anal level: " << this->level;
    this->w->core->analyze(this->level);
}
