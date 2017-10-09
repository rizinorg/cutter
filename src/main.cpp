#include <QApplication>
#include <QCommandLineParser>
#include <QTextCodec>
#include <QMessageBox>

#include "MainWindow.h"
#include "dialogs/NewFileDialog.h"
#include "dialogs/OptionsDialog.h"

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    a.setOrganizationName("cutter");
    a.setApplicationName("cutter");
    a.setApplicationVersion(APP_VERSION);

    // Set QString codec to UTF-8
    QTextCodec::setCodecForLocale(QTextCodec::codecForName("UTF-8"));
#if QT_VERSION < QT_VERSION_CHECK(5,0,0)
    QTextCodec::setCodecForCStrings(QTextCodec::codecForName("UTF-8"));
    QTextCodec::setCodecForTr(QTextCodec::codecForName("UTF-8"));
#endif


    QCommandLineParser cmd_parser;
    cmd_parser.setApplicationDescription(QObject::tr("A Qt and C++ GUI for radare2 reverse engineering framework"));
    cmd_parser.addHelpOption();
    cmd_parser.addVersionOption();
    cmd_parser.addPositionalArgument("filename", QObject::tr("Filename to open."));

    QCommandLineOption anal_option({"A", "anal"},
                                   QObject::tr("Automatically start analysis. Needs filename to be specified. May be a value between 0 and 4."),
                                   QObject::tr("level"));
    cmd_parser.addOption(anal_option);

    cmd_parser.process(a);

    QStringList args = cmd_parser.positionalArguments();

    // Check r2 version
    QString r2version = r_core_version();
    QString localVersion = "" R2_GITTAP;
    if (r2version != localVersion)
    {
        QMessageBox msg;
        msg.setIcon(QMessageBox::Critical);
        msg.setWindowIcon(QIcon(":/img/logo-small.png"));
        msg.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
        msg.setWindowTitle(QObject::tr("Version mismatch!"));
        msg.setText(QString(QObject::tr("The version used to compile cutter (%1) does not match the binary version of radare2 (%2). This could result in unexpected behaviour. Are you sure you want to continue?")).arg(localVersion, r2version));
        if (msg.exec() == QMessageBox::No)
            return 1;
    }

    bool anal_level_specified = false;
    int anal_level = 0;

    if (cmd_parser.isSet(anal_option))
    {
        anal_level = cmd_parser.value(anal_option).toInt(&anal_level_specified);

        if (!anal_level_specified || anal_level < 0 || anal_level > 4)
        {
            printf("%s\n", QObject::tr("Invalid Analysis Level. May be a value between 0 and 4.").toLocal8Bit().constData());
            return 1;
        }
    }


    if (args.empty())
    {
        if (anal_level_specified)
        {
            printf("%s\n", QObject::tr("Filename must be specified to start analysis automatically.").toLocal8Bit().constData());
            return 1;
        }

        NewFileDialog *n = new NewFileDialog();
        n->setAttribute(Qt::WA_DeleteOnClose);
        n->show();
    }
    else // filename specified as positional argument
    {
        MainWindow *main = new MainWindow();
        main->openFile(args[0], anal_level_specified ? anal_level : -1);
    }

    return a.exec();
}
