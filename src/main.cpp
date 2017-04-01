
#include "mainwindow.h"
#include "newfiledialog.h"
#include "optionsdialog.h"

#include <QApplication>
#include <QCommandLineParser>
#include <QTextCodec>
#include <QMessageBox>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    a.setApplicationName("Iaito");
    a.setApplicationVersion(APP_VERSION);

    // Set QString codec to UTF-8
    QTextCodec::setCodecForLocale(QTextCodec::codecForName("UTF-8"));
#if QT_VERSION < QT_VERSION_CHECK(5,0,0)
    QTextCodec::setCodecForCStrings(QTextCodec::codecForName("UTF-8"));
    QTextCodec::setCodecForTr(QTextCodec::codecForName("UTF-8"));
#endif


    QCommandLineParser cmdParser;
    cmdParser.setApplicationDescription("A Qt and C++ GUI for radare2 reverse engineering framework");
    cmdParser.addHelpOption();
    cmdParser.addVersionOption();
    cmdParser.addPositionalArgument("filename", QCoreApplication::translate("main", "Filename to open."));
    cmdParser.process(a);

    QStringList args = cmdParser.positionalArguments();

    // Check r2 version
    QString r2version = r_core_version();
    QString localVersion = "" R2_GITTAP;
    if(r2version != localVersion)
    {
        QMessageBox msg;
        msg.setIcon(QMessageBox::Critical);
        msg.setWindowIcon(QIcon(":/new/prefix1/img/logo-small.png"));
        msg.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
        msg.setWindowTitle("Version mismatch!");
        msg.setText(QString("The version used to compile iaito (%1) does not match the binary version of radare2 (%2). This could result in unexpected behaviour. Are you sure you want to continue?").arg(localVersion, r2version));
        if(msg.exec() == QMessageBox::No)
            return 1;
    }

    if(args.empty())
    {
        NewFileDialog *n = new NewFileDialog();
        n->setAttribute(Qt::WA_DeleteOnClose);
        n->exec();
    }
    else // filename specified as positional argument
    {
        OptionsDialog *o = new OptionsDialog(args[0]);
        o->setAttribute(Qt::WA_DeleteOnClose);
        o->exec();
    }

    return a.exec();
}
