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

    QSignalBlocker s(ui->updatesCheckBox);
    ui->updatesCheckBox->setChecked(Config()->getAutoUpdateEnabled());
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

    connect(&versionChecker, &VersionChecker::checkComplete, &waitDialog, &QProgressDialog::cancel);
    connect(&versionChecker, &VersionChecker::checkComplete,
            [](const QString & version, const QString & error) {
        if (error != "") {
            QMessageBox::critical(nullptr, tr("Error!"), error);
        } else {
            QString text = (version == CUTTER_VERSION_FULL)
                           ? tr("You have latest version and no need to update!")
                           : "<b>" + tr("Current version:") + "</b> " CUTTER_VERSION_FULL "<br/>"
                             + "<b>" + tr("Latest version:") + "</b> " + version + "<br/><br/>"
                             + tr("For update, please check the link:")
                             + "<a href=\"https://github.com/radareorg/cutter/releases\">"
                             + "https://github.com/radareorg/cutter/releases</a>";

            QMessageBox::information(nullptr, tr("Version control"), text);
        }
    });

    versionChecker.checkCurrentVersion(7000);
    waitDialog.exec();
}

void AboutDialog::on_updatesCheckBox_stateChanged(int state)
{
    Config()->setAutoUpdateEnabled(!Config()->getAutoUpdateEnabled());
}
