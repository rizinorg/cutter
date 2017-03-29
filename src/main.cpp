#include "mainwindow.h"
#include "newfiledialog.h"
#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    a.setApplicationVersion(APP_VERSION);

    NewFileDialog n;
    n.show();
    return a.exec();
}
