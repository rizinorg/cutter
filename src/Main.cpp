
#include "CutterApplication.h"
#include "MainWindow.h"
#include <QTranslator>

int main(int argc, char *argv[])
{
    qRegisterMetaType<QList<StringDescription>>();
    qRegisterMetaType<QList<FunctionDescription>>();

    CutterApplication a(argc, argv);

    QTranslator t;
    QString language = Config()->getCurrLanguage();

    if (language == "Русский") {
        t.load(":/translations/cutter_ru.qm");
    }
    a.installTranslator(&t);

    int ret = a.exec();

    return ret;
}
