#include "mainwindow.h"
#include "newfiledialog.h"
#include <QApplication>
#include <QTextCodec>
#include <QMessageBox>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    a.setApplicationVersion(APP_VERSION);

    // Set QString codec to UTF-8
    QTextCodec::setCodecForLocale(QTextCodec::codecForName("UTF-8"));
#if QT_VERSION < QT_VERSION_CHECK(5,0,0)
    QTextCodec::setCodecForCStrings(QTextCodec::codecForName("UTF-8"));
    QTextCodec::setCodecForTr(QTextCodec::codecForName("UTF-8"));
#endif

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

    NewFileDialog n;
    n.show();
    return a.exec();
}
