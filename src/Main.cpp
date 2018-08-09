
#include "CutterApplication.h"
#include "MainWindow.h"

int main(int argc, char *argv[])
{
    qRegisterMetaType<QList<StringDescription>>();
    qRegisterMetaType<QList<FunctionDescription>>();

    CutterApplication a(argc, argv);

    int ret = a.exec();

    return ret;
}
