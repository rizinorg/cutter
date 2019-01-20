
#include "CutterApplication.h"
#include "MainWindow.h"

int main(int argc, char *argv[])
{
    qRegisterMetaType<QList<StringDescription>>();
    qRegisterMetaType<QList<FunctionDescription>>();

    // Application info setup, required to be set before any instance of QSettings will be instantiated
    QCoreApplication::setOrganizationName("Cutter");
    QCoreApplication::setApplicationName("Cutter");

    CutterApplication a(argc, argv);

    int ret = a.exec();

    return ret;
}
