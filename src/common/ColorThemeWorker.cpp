#include "ColorThemeWorker.h"

#include <QDir>
#include <QFile>
#include <QColor>
#include <QJsonArray>
#include <QStandardPaths>

#include "common/Configuration.h"

const QStringList ColorThemeWorker::cutterSpecificOptions = {
    "wordHighlight",
    "lineHighlight",
    "gui.main",
    "gui.imports",
    "highlightPC",
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
    "gui.overview.node",
    "gui.overview.fill",
    "gui.overview.border",
    "gui.border",
    "gui.background",
    "gui.alt_background",
    "gui.disass_selected"
};

const QStringList ColorThemeWorker::radare2UnusedOptions = {
    "linehl",
    "wordhl",
    "graph.box",
    "graph.box2",
    "graph.box3",
    "graph.box4",
    "graph.current",
    "graph.box2",
    "widget_sel",
    "widget_bg",
    "label",
    "ai.write",
    "invalid",
    "ai.seq",
    "args",
    "ai.read",
    "ai.exec",
    "ai.ascii",
    "prompt",
    "graph.traced"
};

ColorThemeWorker::ColorThemeWorker(QObject *parent) : QObject (parent)
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
            tr("Standard themes not found"),
            tr("The radare2 standard themes could not be found. "
               "Most likely, radare2 is not properly installed.")
        );
    }
}

QColor ColorThemeWorker::mergeColors(const QColor& upper, const QColor& lower) const
{
    qreal r1, g1, b1, a1;
    qreal r2, g2, b2, a2;
    qreal r, g, b, a;

    upper.getRgbF(&r1, &g1, &b1, &a1);
    lower.getRgbF(&r2, &g2, &b2, &a2);

    a = (1.0 - a1) * a2 + a1;
    r = ((1.0 - a1) * a2 * r2 + a1 * r1) / a;
    g = ((1.0 - a1) * a2 * g2 + a1 * g1) / a;
    b = ((1.0 - a1) * a2 * b2 + a1 * b1) / a;

    QColor res;
    res.setRgbF(r, g, b, a);
    return res;
}

QString ColorThemeWorker::copy(const QString &srcThemeName,
                                   const QString &copyThemeName) const
{
    if (!isThemeExist(srcThemeName)) {
        return tr("Theme <b>%1</b> does not exist.")
                .arg(srcThemeName);
    }

    return save(getTheme(srcThemeName), copyThemeName);
}

QString ColorThemeWorker::save(const QJsonDocument &theme, const QString &themeName) const
{
    QFile fOut(QDir(customR2ThemesLocationPath).filePath(themeName));
    if (!fOut.open(QFile::WriteOnly | QFile::Truncate)) {
        return tr("The file <b>%1</b> cannot be opened.")
                .arg(QFileInfo(fOut).filePath());
    }

    QJsonObject obj = theme.object();
    QString line;
    QColor::NameFormat format;
    for (auto it = obj.constBegin(); it != obj.constEnd(); it++) {
        if (cutterSpecificOptions.contains(it.key())) {
            line = "#~%1 %2\n";
            format = QColor::HexArgb;
        } else {
            line = "ec %1 %2\n";
            format = QColor::HexRgb;
        }
        QJsonArray arr = it.value().toArray();
        if (arr.isEmpty()) {
            fOut.write(line.arg(it.key())
                       .arg(it.value().toVariant().value<QColor>().name(format)).toUtf8());
        } else if (arr.size() == 4) {
            fOut.write(line.arg(it.key())
                       .arg(QColor(arr[0].toInt(), arr[1].toInt(), arr[2].toInt(), arr[3].toInt()).name(format)).toUtf8());
        } else if (arr.size() == 3) {
            fOut.write(line.arg(it.key())
                       .arg(QColor(arr[0].toInt(), arr[1].toInt(), arr[2].toInt()).name(format)).toUtf8());
        }
    }

    fOut.close();
    return "";
}

bool ColorThemeWorker::isCustomTheme(const QString &themeName) const
{
    return QFile::exists(QDir(customR2ThemesLocationPath).filePath(themeName));
}

bool ColorThemeWorker::isThemeExist(const QString &name) const
{
    QStringList themes = Core()->getColorThemes();
    return themes.contains(name);
}

QJsonDocument ColorThemeWorker::getTheme(const QString& themeName) const
{
    int r, g, b, a;
    QVariantMap theme;
    QString curr = Config()->getColorTheme();

    if (themeName != curr) {
        Core()->cmd(QString("eco %1").arg(themeName));
        theme = Core()->cmdj("ecj").object().toVariantMap();
        Core()->cmd(QString("eco %1").arg(curr));
    } else {
        theme = Core()->cmdj("ecj").object().toVariantMap();
    }

    for (auto it = theme.begin(); it != theme.end(); it++) {
        auto arr = it.value().toList();
        QColor(arr[0].toInt(), arr[1].toInt(), arr[2].toInt()).getRgb(&r, &g, &b, &a);
        theme[it.key()] = QJsonArray({r, g, b, a});
    }

    ColorFlags colorFlags = ColorFlags::DarkFlag;
    if (Configuration::relevantThemes.contains(themeName)) {
        colorFlags = Configuration::relevantThemes[themeName];
    }

    for (auto& it : cutterSpecificOptions) {
        Configuration::cutterOptionColors[it][colorFlags].getRgb(&r, &g, &b, &a);
        theme.insert(it, QJsonArray{r, g, b, a});
    }

    if (isCustomTheme(themeName)) {
        QFile src(QDir(customR2ThemesLocationPath).filePath(themeName));
        if (!src.open(QFile::ReadOnly)) {
            return QJsonDocument();
        }
        QStringList sl;
        for (auto &line : QString(src.readAll()).split('\n', QString::SkipEmptyParts)) {
            sl = line.replace("#~", "ec ").replace("rgb:", "#").split(' ', QString::SkipEmptyParts);
            if (sl.size() != 3 || sl[0][0] == '#') {
                continue;
            }
            QColor(sl[2]).getRgb(&r, &g, &b, &a);
            theme.insert(sl[1], QJsonArray({r, g, b, a}));
        }
    }

    for (auto &key : radare2UnusedOptions) {
        theme.remove(key);
    }

    return QJsonDocument(QJsonObject::fromVariantMap(theme));
}

QString ColorThemeWorker::deleteTheme(const QString &themeName) const
{
    if (!isCustomTheme(themeName)) {
        return tr("You can not delete standard radare2 color themes.");
    }
    if (!isThemeExist(themeName)) {
        return tr("Theme <b>%1</b> does not exist.").arg(themeName);
    }

    QFile file(QDir(customR2ThemesLocationPath).filePath(themeName));
    if (file.isWritable()) {
        return tr("You have no permission to write to <b>%1</b>")
                .arg(QFileInfo(file).filePath());
    }
    if (!file.open(QFile::ReadOnly)) {
        return tr("File <b>%1</b> can not be opened.")
                .arg(QFileInfo(file).filePath());
    }
    if (!file.remove()) {
        return tr("File <b>%1</b> can not be removed.")
                .arg(QFileInfo(file).filePath());
    }
    return "";
}

QString ColorThemeWorker::importTheme(const QString& file) const
{
    QFileInfo src(file);
     if (!src.exists()) {
         return tr("File <b>%1</b> does not exist.").arg(file);
     }

    bool ok;
    bool isTheme = isFileTheme(file, &ok);
    if (!ok) {
        return tr("File <b>%1</b> could not be opened. "
                  "Please make sure you have access to it and try again.")
                .arg(file);
    } else if (!isTheme) {
        return tr("File <b>%1</b> is not a Cutter color theme").arg(file);
    }

    QString name = src.fileName();
    if (isThemeExist(name)) {
        return tr("A color theme named <b>%1</b> already exists.").arg(name);
    }

    if (QFile::copy(file, QDir(customR2ThemesLocationPath).filePath(name))) {
         return "";
     } else {
         return tr("Error occurred during importing. "
                   "Please make sure you have an access to "
                   "the directory <b>%1</b> and try again.")
                 .arg(src.dir().path());
    }
}

QString ColorThemeWorker::renameTheme(const QString& themeName, const QString& newName) const
{
    if (isThemeExist(newName)) {
         return tr("A color theme named <b>\"%1\"</b> already exists.").arg(newName);
     }

     if (!isCustomTheme(themeName)) {
         return tr("You can not rename standard radare2 themes.");
     }

     QDir dir = customR2ThemesLocationPath;
     bool ok = QFile::rename(dir.filePath(themeName), dir.filePath(newName));
     if (!ok) {
         return tr("Something went wrong during renaming. "
                   "Please make sure you have access to the directory <b>\"%1\"</b>.").arg(dir.path());
     }
     return "";
}

bool ColorThemeWorker::isFileTheme(const QString& filePath, bool* ok) const
{
    QFile f(filePath);
    if (!f.open(QFile::ReadOnly)) {
        *ok = false;
        return false;
    }

    const QString colors = "black|red|white|green|magenta|yellow|cyan|blue|gray|none";
    QString options = (Core()->cmdj("ecj").object().keys() << cutterSpecificOptions)
                      .join('|')
                      .replace(".", "\\.");
    QRegExp regexp = QRegExp(QString("((ec\\s+(%1)\\s+(((rgb:|#)[0-9a-fA-F]{3,8})|(%2))))\\s*")
                             .arg(options)
                             .arg(colors));

    for (auto &line : QString(f.readAll()).split('\n', QString::SkipEmptyParts)) {
        line.replace("#~", "ec ");
        if (!line.isEmpty() && !regexp.exactMatch(line)) {
            *ok = true;
            return false;
        }
    }

    *ok = true;
    return true;
}

QStringList ColorThemeWorker::customThemes() const
{
    QStringList themes = Core()->getColorThemes();
    QStringList ret;
    for (int i = 0; i < themes.size(); i++) {
        if (isCustomTheme(themes[i])) {
            ret.push_back(themes[i]);
        }
    }
    return ret;
}
