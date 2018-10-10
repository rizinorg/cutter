#include "utils/ColorSchemeFileSaver.h"

#include <QDir>
#include <QDebug>
#include <QColor>
#include <QJsonArray>
#include <QJsonObject>
#include <QStandardPaths>
#include "utils/Configuration.h"

static const QStringList cutterSpecificOptions = {
    "gui.main",
    "highlight",
    "gui.imports",
    "highlightPC",
    "highlightWord",
    "gui.navbar.err",
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
    QDir currDir;
    QStringList dirs = QStandardPaths::standardLocations(QStandardPaths::GenericDataLocation);
    dirs.removeOne(QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation));

    currDir = QDir(dirs.at(0)).filePath("radare2");
    // currDir.entryList(QDir::Dirs, QDir::Name) returns { current dir, upper dir, dirs ... }
    // so it takes first (and only) dir using .at(2)
    currDir = currDir.filePath(currDir.entryList(QDir::Dirs,
                                                 QDir::Name).at(2) + QDir::separator() + "cons");
    standardR2ThemesLocationPath = currDir.absolutePath();

    customR2ThemesLocationPath = QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation) +
                                 QDir::separator() +
                                 "radare2" + QDir::separator() +
                                 "cons";
    if (!QDir(customR2ThemesLocationPath).exists()) {
        QDir().mkpath(customR2ThemesLocationPath);
    }
}

QFile::FileError ColorSchemeFileSaver::copy(const QString &srcThemeName,
                                            const QString &copyThemeName) const
{
    QFile fIn(standardR2ThemesLocationPath + QDir::separator() + srcThemeName);
    QFile fOut(customR2ThemesLocationPath + QDir::separator() + copyThemeName);

    if (srcThemeName != "default" && !fIn.open(QFile::ReadOnly)) {
        fIn.setFileName(customR2ThemesLocationPath + QDir::separator() + srcThemeName);
        if (!fIn.open(QFile::ReadOnly)) {
            return fIn.error();
        }
    }

    if (!fOut.open(QFile::WriteOnly | QFile::Truncate)) {
        fIn.close();
        return fOut.error();
    }

    QStringList options = Core()->cmdj("ecj").object().keys();
    options << cutterSpecificOptions;
    QStringList src;

    if (srcThemeName == "default") {
        QString theme = Config()->getCurrentTheme();
        Core()->cmd("ecd");
        QJsonObject _obj = Core()->cmdj("ecj").object();
        Core()->cmd(QString("eco %1").arg(theme));
        QColor back = Config()->getColor(standardBackgroundOptionName);
        _obj[standardBackgroundOptionName] = QJsonArray({back.red(), back.green(), back.blue()});
        for (auto &it : _obj.keys()) {
            QJsonArray rgb = _obj[it].toArray();
            src.push_back("ec " + it + " " +
                          QColor(rgb[0].toInt(), rgb[1].toInt(), rgb[2].toInt()).name().replace("#", "rgb:"));
        }
    } else {
        src = QString(fIn.readAll()).split('\n');
    }

    for (auto &it : src) {
        if (it.isEmpty()) {
            continue;
        }
        fOut.write(it.toUtf8() + '\n');

        if (it.length() > 2 && it.left(2) == "#~") {
            options.removeOne(it.split(' ')[0].remove("#~").toUtf8());
        } else {
            options.removeOne(it.split(' ').at(1));
        }
    }

    for (auto &it : options) {
        if (cutterSpecificOptions.contains(it))  {
            fOut.write("#~");
        } else {
            fOut.write("ec ");
        }
        fOut.write(QString(it + " rgb:%1\n").
                   arg(Config()->getColor(it).name().remove("#")).toUtf8());
    }

    fOut.close();
    fIn.close();

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
    for (auto &it : QDir(customR2ThemesLocationPath).entryInfoList())
        if (it.fileName() == schemeName)
            return true;
    return false;
}

bool ColorSchemeFileSaver::isNameEngaged(const QString &name) const
{
    return QFile::exists(standardR2ThemesLocationPath + QDir::separator() + name) ||
           QFile::exists(customR2ThemesLocationPath + QDir::separator() + name);
}

QMap<QString, QColor> ColorSchemeFileSaver::getCutterSpecific() const
{
    QFile f(customR2ThemesLocationPath + QDir::separator() + Config()->getCurrentTheme());
    if (!f.open(QFile::ReadOnly))
        return  QMap<QString, QColor>();

    QMap<QString, QColor> ret;
    QStringList data = QString(f.readAll()).split('\n');
    for (auto &it : data) {
        if (it.length() > 2 && it.left(2) == "#~") {
            QStringList currLine = it.split(' ');
            ret.insert(currLine[0].remove("#~"), currLine[1].replace("rgb:", "#"));
        }
    }

    f.close();
    return ret;
}

QStringList ColorSchemeFileSaver::getCustomSchemes() const
{
    QStringList sl;
    sl = QDir(customR2ThemesLocationPath).entryList(QDir::Files, QDir::Name);
    sl.removeOne(".");
    sl.removeOne("..");
    return sl;
}

void ColorSchemeFileSaver::deleteScheme(const QString &schemeName) const
{
    if (!isCustomScheme(schemeName))
        return;
    QFile::remove(customR2ThemesLocationPath + QDir::separator() + schemeName);
}
