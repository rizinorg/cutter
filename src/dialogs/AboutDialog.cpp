#include <Cutter.h>
#include "AboutDialog.h"
#include "ui_AboutDialog.h"
#include "R2PluginsDialog.h"
#include "r_version.h"
#include "utils/Configuration.h"

AboutDialog::AboutDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::AboutDialog)
{
    ui->setupUi(this);
    setWindowFlags(windowFlags() & (~Qt::WindowContextHelpButtonHint));
    ui->logoSvgWidget->load(Config()->getLogoFile());

    ui->label->setText(tr("<h1>Cutter</h1>"
                          "Version " CUTTER_VERSION "<br/>"
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
