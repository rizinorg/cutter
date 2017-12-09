#include <QApplication>
#include <QCommandLineParser>
#include <QTextCodec>
#include <QMessageBox>

#include "MainWindow.h"
#include "dialogs/NewFileDialog.h"
#include "dialogs/OptionsDialog.h"

#ifdef APPIMAGE
#define PREFIX "/tmp/.cutter_usr"
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>

void set_appimage_symlink()
{
    char *path = realpath("/proc/self/exe", NULL);
    char *i = strrchr(path, '/');
    *(i + 1) = '\0';
    char *dest = strcat(path, "../");
    struct stat buf;
    if (lstat(PREFIX, &buf) == 0 && S_ISLNK(buf.st_mode))
    {
        remove(PREFIX);
    }
    symlink(dest, PREFIX);
    printf("'%s' '%s' '%s'\n", path, i, dest);
    free(path);
}
#endif

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    a.setOrganizationName("cutter");
    a.setApplicationName("cutter");
    a.setApplicationVersion(APP_VERSION);
    a.setWindowIcon(QIcon(":/img/cutter.svg"));

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
                                   QObject::tr("Automatically open file and optionally start analysis. Needs filename to be specified. May be a value between 0 and 2: 0 = no analysis, 1 = aaa, 2 = aaaa (experimental)"),
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

        if (!anal_level_specified || anal_level < 0 || anal_level > 2)
        {
            printf("%s\n", QObject::tr("Invalid Analysis Level. May be a value between 0 and 2.").toLocal8Bit().constData());
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
        main->openNewFile(args[0], anal_level_specified ? anal_level : -1);
    }

    // Hack to make it work with AppImage
#ifdef APPIMAGE
    set_appimage_symlink();
#endif

    int ret = a.exec();

#ifdef APPIMAGE
    remove(PREFIX);
#endif

    return ret;
}
