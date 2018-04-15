
#include "CutterApplication.h"
#include "MainWindow.h"

int main(int argc, char *argv[])
{
    CutterApplication a(argc, argv);

    int ret = a.exec();

    return ret;
}
