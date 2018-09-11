#include <Cutter.h>
#include "AboutDialog.h"
#include "ui_AboutDialog.h"
#include "R2PluginsDialog.h"
#include "r_version.h"
#include "utils/Configuration.h"

#include <QtNetwork/QNetworkReply>
#include <QtNetwork/QNetworkRequest>
#include <QtNetwork/QNetworkAccessManager>
#include <QUrl>
#include <QDebug>
#include <QTimer>
#include <QProgressBar>
#include <QProgressDialog>

#include "CutterConfig.h"

AboutDialog::AboutDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::AboutDialog)
{
    ui->setupUi(this);
    setWindowFlags(windowFlags() & (~Qt::WindowContextHelpButtonHint));
    ui->logoSvgWidget->load(Config()->getLogoFile());

    ui->label->setText(tr("<h1>Cutter</h1>"
                          "Version " CUTTER_VERSION_FULL "<br/>"
                          "Using r2-" R2_GITTAP
                          "<p><b>Optional Features:</b><br/>"
                          "Jupyter: %1<br/>"
                          "QtWebEngine: %2</p>"
                          "<h2>License</h2>"
                          "This Software is released under the GNU General Public License v3.0"
                          "<h2>Authors</h2>"
                          "xarkes, thestr4ng3r, ballessay<br/>"
                          "Based on work by Hugo Teso &lt;hugo.teso@gmail.org&gt; (originally Iaito).")
                       .arg(
#ifdef CUTTER_ENABLE_JUPYTER
                           "ON"
#else
                           "OFF"
#endif
                           ,
#ifdef CUTTER_ENABLE_QTWEBENGINE
                           "ON"
#else
                           "OFF"
#endif
                       ));
}

AboutDialog::~AboutDialog() {}

void AboutDialog::on_buttonBox_rejected()
{
    close();
}

void AboutDialog::on_showVersionButton_clicked()
{
    QMessageBox popup(this);
    popup.setWindowTitle("radare2 version information");
    auto versionInformation = Core()->getVersionInformation();
    popup.setText(versionInformation);
    popup.exec();
}

void AboutDialog::on_showPluginsButton_clicked()
{
    R2PluginsDialog dialog(this);
    dialog.exec();
}

void AboutDialog::on_checkForUpdatesButton_clicked()
{
    QString currVersion = "";
    QUrl url("https://github.com/radareorg/cutter/releases/latest");
    QNetworkRequest request;
    request.setUrl(url);

    QProgressDialog waitDialog;
    QProgressBar* bar = new QProgressBar(&waitDialog);
    bar->setMaximum(0);

    waitDialog.setBar(bar);
    waitDialog.setLabel(new QLabel("Checking for updates...", &waitDialog));

    QNetworkAccessManager nm;

    connect(&nm, &QNetworkAccessManager::finished,
    [&currVersion, &waitDialog](QNetworkReply * reply) {
        waitDialog.cancel();
        QMessageBox mb;
        if (reply->error()) {
            mb.setIcon(QMessageBox::Critical);
            mb.setStandardButtons(QMessageBox::Ok);
            mb.setWindowTitle(QObject::tr("Error!"));
            mb.setText(reply->errorString());
        } else {
            currVersion = reply->readAll();
            currVersion.remove(0, currVersion.indexOf("/tag/") + 6);
            currVersion = currVersion.left(currVersion.indexOf('>') - 1);
            if (currVersion == CUTTER_VERSION_FULL) {
                mb.setIcon(QMessageBox::Information);
                mb.setStandardButtons(QMessageBox::Ok);
                mb.setWindowTitle(QObject::tr("Version control"));
                mb.setText(QObject::tr("You have latest version and no need to update!"));
            } else {
                mb.setIcon(QMessageBox::Information);
                mb.setStandardButtons(QMessageBox::Ok);
                mb.setWindowTitle(QObject::tr("Version control"));
                mb.setText(tr("<b>Current version</b>: " CUTTER_VERSION_FULL "<br/>"
                              "<b>Latest version</b>: %1<br/><br/>"
                              "For update, please check the link: <a href=\"%2\">%2</a>")
                           .arg(currVersion, "https://github.com/radareorg/cutter/releases"));
            }
        }
        mb.exec();
    });

    QTimer::singleShot(15000, this, [&currVersion, &waitDialog]() {
        if (currVersion.isEmpty()) {
            waitDialog.cancel();
            QMessageBox mb;
            mb.setIcon(QMessageBox::Critical);
            mb.setStandardButtons(QMessageBox::Ok);
            mb.setWindowTitle(QObject::tr("Timeout error!"));
            mb.setText(tr("Please check your internet connection and try again."));
            mb.exec();
        }
    });

    nm.get(request);
    waitDialog.exec();
}
