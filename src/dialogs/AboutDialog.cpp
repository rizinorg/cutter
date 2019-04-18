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
#include <UpdateWorker.h>
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
                        + QString("Python: %1<br/>").arg(
#ifdef CUTTER_ENABLE_PYTHON
                            "ON"
#else
                            "OFF"
#endif
                        )
                        + QString("Python Bindings: %2</p>").arg(
#ifdef CUTTER_ENABLE_PYTHON_BINDINGS
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
    UpdateWorker updateWorker;

    QProgressDialog waitDialog;
    QProgressBar *bar = new QProgressBar(&waitDialog);
    bar->setMaximum(0);

    waitDialog.setBar(bar);
    waitDialog.setLabel(new QLabel(tr("Checking for updates..."), &waitDialog));

    connect(&updateWorker, &UpdateWorker::checkComplete, &waitDialog, &QProgressDialog::cancel);
    connect(&updateWorker, &UpdateWorker::checkComplete,
    [&updateWorker](const QVersionNumber &version, const QString & error) {
        if (!error.isEmpty()) {
            QMessageBox::critical(nullptr, tr("Error!"), error);
        } else {
            if (version <= UpdateWorker::currentVersionNumber()) {
                QMessageBox::information(nullptr, tr("Version control"), tr("Cutter is up to date!"));
            } else {
                updateWorker.showUpdateDialog(false);
            }
        }
    });

    updateWorker.checkCurrentVersion(7000);
    waitDialog.exec();
}

void AboutDialog::on_updatesCheckBox_stateChanged(int)
{
    Config()->setAutoUpdateEnabled(!Config()->getAutoUpdateEnabled());
}
