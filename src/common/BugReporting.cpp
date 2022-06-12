#include "BugReporting.h"

#include "Cutter.h"
#include <QUrl>
#include <QJsonObject>
#include "CutterConfig.h"
#include <QDesktopServices>

void openIssue()
{
    RzCoreLocked core(Core());
    RzBinFile *bf = rz_bin_cur(core->bin);
    RzBinInfo *info = rz_bin_get_info(core->bin);
    RzBinPlugin *plugin = rz_bin_file_cur_plugin(bf);

    QString url, osInfo, format, arch, type;
    // Pull in info needed for git issue
    osInfo = QSysInfo::productType() + " "
            + (QSysInfo::productVersion() == "unknown" ? "" : QSysInfo::productVersion());
    format = plugin && RZ_STR_ISNOTEMPTY(plugin->name) ? plugin->name : "N/A";
    arch = info && RZ_STR_ISNOTEMPTY(info->arch) ? info->arch : "N/A";
    type = info && RZ_STR_ISNOTEMPTY(info->type) ? info->type : "N/A";
    url = "https://github.com/rizinorg/cutter/issues/new?&body=**Environment information**\n* "
          "Operating System: "
            + osInfo + "\n* Cutter version: " + CUTTER_VERSION_FULL + "\n* Obtained from:\n"
            + "  - [x] Built from source\n  - [ ] Downloaded release from Cutter website or GitHub "
              "\n"
              "  - [ ] Distribution repository\n* File format: "
            + format + "\n * Arch: " + arch + "\n * Type: " + type
            + "\n\n**Describe the bug**\n\n<!-- A clear and concise description of what the bug "
              "is. -->"
              "\n\n**To Reproduce**\n\n"
              "Steps to reproduce the behavior:\n1. Go to '...'\n2. Click on '....'\n3. Scroll "
              "down to '....'\n"
              "4. See error\n\n**Expected behavior**\n\n"
              "<!-- A clear and concise description of what you expected to happen. -->\n\n\n"
              "**Screenshots**\n\n<!-- If applicable, add screenshots to help explain your "
              "problem. -->\n\n\n"
              "**Additional context**\n\n<!-- Add any other context about the problem here. -->\n";

    QDesktopServices::openUrl(QUrl(url, QUrl::TolerantMode));
}
