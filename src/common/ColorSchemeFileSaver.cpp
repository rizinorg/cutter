#include "common/ColorSchemeFileSaver.h"

#include <QRgb>
#include <QDir>
#include <QDebug>
#include <QColor>
#include <QJsonArray>
#include <QJsonObject>
#include <QStandardPaths>
#include "common/Configuration.h"

static const QStringList cutterSpecificOptions = {
    "gui.main",
    "highlight",
    "gui.imports",
    "highlightPC",
    "highlightWord",
    "gui.navbar.err",
    "gui.navbar.seek",
    "gui.navbar.pc",
    "gui.navbar.sym",
    "gui.dataoffset",
    "gui.navbar.code",
    "gui.navbar.empty",
    "angui.navbar.str",
    "gui.disass_selected",
    "gui.breakpoint_background",
    "gui.overview.node"
};

ColorSchemeFileSaver::ColorSchemeFileSaver(QObject *parent) : QObject (parent)
{
    char* szThemes = r_str_home(R2_HOME_THEMES);
    customR2ThemesLocationPath = szThemes;
    r_mem_free(szThemes);
    if (!QDir(customR2ThemesLocationPath).exists()) {
        QDir().mkpath(customR2ThemesLocationPath);
    }

    QDir currDir { QStringLiteral("%1%2%3")
        .arg(r_sys_prefix(nullptr))
        .arg(R_SYS_DIR)
        .arg(R2_THEMES)
    };
    if (currDir.exists()) {
        standardR2ThemesLocationPath = currDir.absolutePath();
    } else {
        QMessageBox::critical(nullptr,
            tr("Standard themes not found!"),
            tr("The radare2 standard themes could not be found! This probably means radare2 is not properly installed. If you think it is open an issue please.")
        );
    }
}

QString ColorSchemeFileSaver::copy(const QString &srcThemeName,
                                   const QString &copyThemeName) const
{
    if (!isSchemeExist(srcThemeName)) {
        return tr("TODO");
    }
    QString currTheme = Config()->getColorTheme();
    Config()->setColorTheme(srcThemeName);

    QJsonObject scheme;
    QStringList options = Core()->cmdj("ecj").object().keys() << cutterSpecificOptions;
    for (auto &currOption : options) {
        QString color = Config()->getColor(currOption).name();
        scheme.insert(currOption, color);
    }

    Config()->setColorTheme(currTheme);

    return save(QJsonDocument(scheme), copyThemeName);
}

QString ColorSchemeFileSaver::save(const QJsonDocument &scheme, const QString &schemeName) const
{
    QFile fOut(customR2ThemesLocationPath + QDir::separator() + schemeName);
    if (!fOut.open(QFile::WriteOnly | QFile::Truncate)) {
        return "TODO";
    }

    QJsonObject obj = scheme.object();
    for (auto it = obj.constBegin(); it != obj.constEnd(); it++) {
        QString line;
        if (cutterSpecificOptions.contains(it.key())) {
            line = "#~%1 %2\n";
        } else {
            line = "ec %1 %2\n";
        }
        QJsonArray arr = it.value().toArray();
        if (arr.isEmpty()) {
            fOut.write(line.arg(it.key())
                       .arg(it.value().toVariant().value<QColor>().name()).toUtf8());
        } else if (arr.size() == 3) {
            fOut.write(line.arg(it.key())
                       .arg(QColor(arr[0].toInt(), arr[1].toInt(), arr[2].toInt()).name()).toUtf8());
        }
    }

    fOut.close();
    return "";
}

bool ColorSchemeFileSaver::isCustomScheme(const QString &schemeName) const
{
    return QFile::exists(QDir(customR2ThemesLocationPath).filePath(schemeName));
}

bool ColorSchemeFileSaver::isSchemeExist(const QString &name) const
{
    return (!standardR2ThemesLocationPath.isEmpty() && QFile::exists(standardR2ThemesLocationPath + QDir::separator() + name)) ||
           QFile::exists(customR2ThemesLocationPath + QDir::separator() + name);
}

QMap<QString, QColor> ColorSchemeFileSaver::getCutterSpecific() const
{
    QMap<QString, QColor> ret;
    for (auto &currOption : cutterSpecificOptions) {
        ret.insert(currOption, Config()->getColor(currOption));
    }
    return ret;
}

QStringList ColorSchemeFileSaver::getCustomSchemes() const
{
    return QDir(customR2ThemesLocationPath).entryList(QDir::Files | QDir::NoDotAndDotDot, QDir::Name);
}

void ColorSchemeFileSaver::deleteScheme(const QString &schemeName) const
{
    if (!isCustomScheme(schemeName))
        return;
    QFile::remove(customR2ThemesLocationPath + QDir::separator() + schemeName);
}
