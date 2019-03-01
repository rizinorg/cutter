#include "r_version.h"
#include "core/Cutter.h"
#include "AboutDialog.h"

#include "ui_AboutDialog.h"
#include "R2PluginsDialog.h"
#include "common/Configuration.h"

#include <QUrl>
#include <QTimer>
#include <QEventLoop>
#include <QJsonObject>
#include <QProgressBar>
#include <QProgressDialog>
#include <VersionChecker.h>
#include <QtNetwork/QNetworkRequest>
#include <QtNetwork/QNetworkAccessManager>

#include "CutterConfig.h"

AboutDialog::AboutDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::AboutDialog)
{
    ui->setupUi(this);
    setWindowFlags(windowFlags() & (~Qt::WindowContextHelpButtonHint));
    ui->logoSvgWidget->load(Config()->getLogoFile());

    QString aboutString("<h1>Cutter</h1>"
                          + tr("Version") + " " CUTTER_VERSION_FULL "<br/>"
                          + tr("Using r2-") + R2_GITTAP
                          + "<p><b>" + tr("Optional Features:") + "</b><br/>"
                          + QString("Jupyter: %1<br/>").arg(
#ifdef CUTTER_ENABLE_JUPYTER
                          "ON"
#else
                          "OFF"
#endif
                           )
                          + QString("QtWebEngine: %2</p>").arg(
#ifdef CUTTER_ENABLE_QTWEBENGINE
                          "ON"
#else
                          "OFF"
#endif
                           )
                          + "<h2>" + tr("License") + "</h2>"
                          + tr("This Software is released under the GNU General Public License v3.0")
                          + "<h2>" + tr("Authors") + "</h2>"
                          "xarkes, thestr4ng3r, ballessay<br/>"
                          "Based on work by Hugo Teso &lt;hugo.teso@gmail.org&gt; (originally Iaito).");
    ui->label->setText(aboutString);
}

AboutDialog::~AboutDialog() {}

void AboutDialog::on_buttonBox_rejected()
{
    close();
}

void AboutDialog::on_showVersionButton_clicked()
{
    QMessageBox popup(this);
    popup.setWindowTitle(tr("radare2 version information"));
    popup.setTextInteractionFlags(Qt::TextSelectableByMouse);
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
    VersionChecker versionChecker;

    QProgressDialog waitDialog;
    QProgressBar *bar = new QProgressBar(&waitDialog);
    bar->setMaximum(0);

    waitDialog.setBar(bar);
    waitDialog.setLabel(new QLabel(tr("Checking for updates..."), &waitDialog));

    connect(&versionChecker, &VersionChecker::checkComplete, this, &AboutDialog::serveVersionCheckReply);
    connect(&versionChecker, &VersionChecker::checkComplete, &waitDialog, &QProgressDialog::cancel);

    versionChecker.checkCurrentVersion(7000, [&]() {
        waitDialog.cancel();
        QMessageBox::critical(this,
                              tr("Timeout error!"),
                              tr("Time limit exceeded during version check. Please check your "
                                 "internet connection and try again."));
    });
    waitDialog.exec();
    disconnect(&versionChecker, 0, 0, 0);
}

void AboutDialog::serveVersionCheckReply(const QString& version, const QString& error)
{
    if (error != "") {
        QMessageBox::critical(this, tr("Error!"), error);
    } else {
        QMessageBox mb;
        mb.setStandardButtons(QMessageBox::Ok);
        mb.setWindowTitle(tr("Version control"));
        mb.setIcon(QMessageBox::Information);
        if (version == CUTTER_VERSION_FULL) {
            mb.setText(tr("You have latest version and no need to update!"));
        } else {
            mb.setText("<b>" + tr("Current version:") + "</b> " CUTTER_VERSION_FULL "<br/>"
                          + "<b>" + tr("Latest version:") + "</b> " + version + "<br/><br/>"
                          + tr("For update, please check the link:")
                          + "<a href=\"https://github.com/radareorg/cutter/releases\">"
                          + "https://github.com/radareorg/cutter/releases</a>");
        }
        mb.exec();
    }
}
