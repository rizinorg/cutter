#include "common/ColorSchemeFileSaver.h"

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
    "gui.breakpoint_background"
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

QFile::FileError ColorSchemeFileSaver::copy(const QString &srcThemeName,
                                            const QString &copyThemeName) const
{
    QFile fIn(standardR2ThemesLocationPath + QDir::separator() + srcThemeName);
    QFile fOut(customR2ThemesLocationPath + QDir::separator() + copyThemeName);

    if (srcThemeName != QStringLiteral("default") && !fIn.open(QFile::ReadOnly)) {
        fIn.setFileName(customR2ThemesLocationPath + QDir::separator() + srcThemeName);
        if (!fIn.open(QFile::ReadOnly)) {
            return fIn.error();
        }
    }

    const QString &srcTheme = fIn.readAll();
    fIn.close();

    if (!fOut.open(QFile::WriteOnly | QFile::Truncate)) {
        return fOut.error();
    }

    QStringList options = Core()->cmdj("ecj").object().keys();
    options << cutterSpecificOptions;
    QStringList src;

    if (srcThemeName == "default") {
        const QString &theme = Config()->getColorTheme();
        Core()->cmd("ecd");
        QJsonObject obj = Core()->cmdj("ecj").object();
        Core()->cmd(QStringLiteral("eco %1").arg(theme));
        QColor back = Config()->getColor(standardBackgroundOptionName);
        obj[standardBackgroundOptionName] = QJsonArray({back.red(), back.green(), back.blue()});
        for (const QString &it : obj.keys()) {
            QJsonArray rgb = obj[it].toArray();
            if (rgb.size() != 3) {
                continue;
            }
            src.push_back(QStringLiteral("ec %1 rgb:%2")
                .arg(it)
                .arg(QColor(rgb[0].toInt(), rgb[1].toInt(), rgb[2].toInt()).name().remove('#')));
        }
    } else {
        src = srcTheme.split('\n');
    }

    QStringList tmp;
    for (const QString &it : src) {
        if (it.isEmpty()) {
            continue;
        }
        fOut.write(it.toUtf8() + '\n');

        tmp = it.split(' ');
        if (it.length() > 2 && it.left(2) == "#~") {
            options.removeOne(tmp[0].remove("#~").toUtf8());
        } else if (tmp.size() > 1) {
            options.removeOne(tmp.at(1));
        }
    }

    for (const QString &it : options) {
        if (cutterSpecificOptions.contains(it))  {
            fOut.write("#~");
        } else {
            fOut.write("ec ");
        }
        fOut.write(QStringLiteral("%1 rgb:%2\n")
            .arg(it)
            .arg(Config()->getColor(it).name().remove('#')).toUtf8());
    }

    fOut.close();

    return QFile::FileError::NoError;
}

QFile::FileError ColorSchemeFileSaver::save(const QString &scheme, const QString &schemeName) const
{
    QFile fOut(customR2ThemesLocationPath + QDir::separator() + schemeName);
    if (!fOut.open(QFile::WriteOnly | QFile::Truncate))
        return fOut.error();

    fOut.write(scheme.toUtf8());
    fOut.close();
    return QFile::FileError::NoError;
}

bool ColorSchemeFileSaver::isCustomScheme(const QString &schemeName) const
{
    return QFile::exists(QDir(customR2ThemesLocationPath).filePath(schemeName));
}

bool ColorSchemeFileSaver::isNameEngaged(const QString &name) const
{
    return (!standardR2ThemesLocationPath.isEmpty() && QFile::exists(standardR2ThemesLocationPath + QDir::separator() + name)) ||
           QFile::exists(customR2ThemesLocationPath + QDir::separator() + name);
}

QMap<QString, QColor> ColorSchemeFileSaver::getCutterSpecific() const
{
    QFile f(customR2ThemesLocationPath + QDir::separator() + Config()->getColorTheme());
    if (!f.open(QFile::ReadOnly))
        return  QMap<QString, QColor>();

    const QStringList &data = QString(f.readAll()).split('\n');
    f.close();

    QMap<QString, QColor> ret;
    for (const QString &it : data) {
        if (it.length() > 2 && it.left(2) == "#~") {
            QStringList currLine = it.split(' ');
            if (currLine.size() > 1) {
                ret.insert(currLine[0].remove("#~"), currLine[1].replace("rgb:", "#"));
            }
        }
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
