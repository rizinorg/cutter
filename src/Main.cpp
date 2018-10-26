
#include "CutterApplication.h"
#include "MainWindow.h"
#include <QTranslator>
#include <QDir>

int main(int argc, char *argv[])
{
    qRegisterMetaType<QList<StringDescription>>();
    qRegisterMetaType<QList<FunctionDescription>>();

    CutterApplication a(argc, argv);

    QTranslator t;
    QString language = Config()->getCurrLanguage().toLower();
    auto allLocales = QLocale::matchingLocales(QLocale::AnyLanguage, QLocale::AnyScript,
                                               QLocale::AnyCountry);

    if (language != "English") {
        for (auto &it : allLocales) {
            if (it.nativeLanguageName() == language) {
                QString langPrefix = it.bcp47Name().left(2);
                t.load(QString(QCoreApplication::applicationDirPath() + QDir::separator() +
                               "translations" + QDir::separator() +
                               "cutter_%1.qm").arg(langPrefix));
                a.installTranslator(&t);
                break;
            }
        }
    }

    int ret = a.exec();

    return ret;
}
