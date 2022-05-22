#include "ColorThemeWorker.h"

#include <QDir>
#include <QFile>
#include <QColor>
#include <QJsonArray>
#include <QStandardPaths>
#include <QRegularExpression>
#include <rz_util/rz_path.h>

#include "common/Configuration.h"

const QStringList ColorThemeWorker::cutterSpecificOptions = {
    "wordHighlight",      "lineHighlight",       "gui.main",
    "gui.imports",        "highlightPC",         "gui.navbar.err",
    "gui.navbar.seek",    "gui.navbar.pc",       "gui.navbar.sym",
    "gui.dataoffset",     "gui.navbar.code",     "gui.navbar.empty",
    "angui.navbar.str",   "gui.disass_selected", "gui.breakpoint_background",
    "gui.overview.node",  "gui.overview.fill",   "gui.overview.border",
    "gui.border",         "gui.background",      "gui.alt_background",
    "gui.disass_selected"
};

const QStringList ColorThemeWorker::rizinUnusedOptions = {
    "linehl",     "wordhl",        "graph.box",  "graph.box2", "graph.box3",
    "graph.box4", "graph.current", "graph.box2", "widget_sel", "widget_bg",
    "label",      "ai.write",      "invalid",    "ai.seq",     "args",
    "ai.read",    "ai.exec",       "ai.ascii",   "prompt",     "graph.traced"
};

ColorThemeWorker::ColorThemeWorker(QObject *parent) : QObject(parent)
{
    char *szThemes = rz_path_home_prefix(RZ_THEMES);
    customRzThemesLocationPath = szThemes;
    rz_mem_free(szThemes);
    if (!QDir(customRzThemesLocationPath).exists()) {
        QDir().mkpath(customRzThemesLocationPath);
    }

    QDir currDir { QStringLiteral("%1%2%3").arg(rz_path_prefix(nullptr), RZ_SYS_DIR, RZ_THEMES) };
    if (currDir.exists()) {
        standardRzThemesLocationPath = currDir.absolutePath();
    } else {
        QMessageBox::critical(nullptr, tr("Standard themes not found"),
                              tr("The Rizin standard themes could not be found in '%1'. "
                                 "Most likely, Rizin is not properly installed.")
                                      .arg(currDir.path()));
    }
}

QColor ColorThemeWorker::mergeColors(const QColor &upper, const QColor &lower) const
{
    qhelpers::ColorFloat r1, g1, b1, a1;
    qhelpers::ColorFloat r2, g2, b2, a2;
    qhelpers::ColorFloat r, g, b, a;

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

QString ColorThemeWorker::copy(const QString &srcThemeName, const QString &copyThemeName) const
{
    if (!isThemeExist(srcThemeName)) {
        return tr("Theme <b>%1</b> does not exist.").arg(srcThemeName);
    }

    return save(getTheme(srcThemeName), copyThemeName);
}

QString ColorThemeWorker::save(const Theme &theme, const QString &themeName) const
{
    QFile fOut(QDir(customRzThemesLocationPath).filePath(themeName));
    if (!fOut.open(QFile::WriteOnly | QFile::Truncate)) {
        return tr("The file <b>%1</b> cannot be opened.").arg(QFileInfo(fOut).filePath());
    }

    for (auto it = theme.constBegin(); it != theme.constEnd(); it++) {
        const QColor &color = it.value();
        if (cutterSpecificOptions.contains(it.key())) {
            fOut.write(QString("#~%1 rgb:%2\n")
                               .arg(it.key(), color.name(QColor::HexArgb).remove('#'))
                               .toUtf8());
        } else {
            fOut.write(QString("ec %1 rgb:%2\n")
                               .arg(it.key(), color.name(QColor::HexRgb).remove('#'))
                               .toUtf8());
        }
    }

    fOut.close();
    return "";
}

bool ColorThemeWorker::isCustomTheme(const QString &themeName) const
{
    return QFile::exists(QDir(customRzThemesLocationPath).filePath(themeName));
}

bool ColorThemeWorker::isThemeExist(const QString &name) const
{
    QStringList themes = Core()->getColorThemes();
    return themes.contains(name);
}

ColorThemeWorker::Theme ColorThemeWorker::getTheme(const QString &themeName) const
{
    Theme theme;
    QString curr = Config()->getColorTheme();

    if (themeName != curr) {
        RzCoreLocked core(Core());
        rz_core_theme_load(core, themeName.toUtf8().constData());
        theme = Core()->getTheme();
        rz_core_theme_load(core, curr.toUtf8().constData());
    } else {
        theme = Core()->getTheme();
    }

    ColorFlags colorFlags = ColorFlags::DarkFlag;
    if (Configuration::relevantThemes.contains(themeName)) {
        colorFlags = Configuration::relevantThemes[themeName];
    }

    for (auto &it : cutterSpecificOptions) {
        theme.insert(it, QColor(Configuration::cutterOptionColors[it][colorFlags]));
    }

    if (isCustomTheme(themeName)) {
        QFile src(QDir(customRzThemesLocationPath).filePath(themeName));
        if (!src.open(QFile::ReadOnly)) {
            return {};
        }
        QStringList sl;
        for (auto &line : QString(src.readAll()).split('\n', CUTTER_QT_SKIP_EMPTY_PARTS)) {
            sl = line.replace("#~", "ec ")
                         .replace("rgb:", "#")
                         .split(' ', CUTTER_QT_SKIP_EMPTY_PARTS);
            if (sl.size() != 3 || sl[0][0] == '#') {
                continue;
            }
            theme.insert(sl[1], QColor(sl[2]));
        }
    }

    for (auto &key : rizinUnusedOptions) {
        theme.remove(key);
    }

    return theme;
}

QString ColorThemeWorker::deleteTheme(const QString &themeName) const
{
    if (!isCustomTheme(themeName)) {
        return tr("You can not delete standard Rizin color themes.");
    }
    if (!isThemeExist(themeName)) {
        return tr("Theme <b>%1</b> does not exist.").arg(themeName);
    }

    QFile file(QDir(customRzThemesLocationPath).filePath(themeName));
    if (file.isWritable()) {
        return tr("You have no permission to write to <b>%1</b>").arg(QFileInfo(file).filePath());
    }
    if (!file.open(QFile::ReadOnly)) {
        return tr("File <b>%1</b> can not be opened.").arg(QFileInfo(file).filePath());
    }
    if (!file.remove()) {
        return tr("File <b>%1</b> can not be removed.").arg(QFileInfo(file).filePath());
    }
    return "";
}

QString ColorThemeWorker::importTheme(const QString &file) const
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

    if (QFile::copy(file, QDir(customRzThemesLocationPath).filePath(name))) {
        return "";
    } else {
        return tr("Error occurred during importing. "
                  "Please make sure you have an access to "
                  "the directory <b>%1</b> and try again.")
                .arg(src.dir().path());
    }
}

QString ColorThemeWorker::renameTheme(const QString &themeName, const QString &newName) const
{
    if (isThemeExist(newName)) {
        return tr("A color theme named <b>\"%1\"</b> already exists.").arg(newName);
    }

    if (!isCustomTheme(themeName)) {
        return tr("You can not rename standard Rizin themes.");
    }

    QDir dir = customRzThemesLocationPath;
    bool ok = QFile::rename(dir.filePath(themeName), dir.filePath(newName));
    if (!ok) {
        return tr("Something went wrong during renaming. "
                  "Please make sure you have access to the directory <b>\"%1\"</b>.")
                .arg(dir.path());
    }
    return "";
}

bool ColorThemeWorker::isFileTheme(const QString &filePath, bool *ok) const
{
    QFile f(filePath);
    if (!f.open(QFile::ReadOnly)) {
        *ok = false;
        return false;
    }

    const QString colors = "black|red|white|green|magenta|yellow|cyan|blue|gray|none";
    QString options =
            (Core()->getThemeKeys() << cutterSpecificOptions).join('|').replace(".", "\\.");

    QString pattern =
            QString("((ec\\s+(%1)\\s+(((rgb:|#)[0-9a-fA-F]{3,8})|(%2))))\\s*").arg(options, colors);
    // The below construct mimics the behaviour of QRegexP::exactMatch(), which was here before
    QRegularExpression regexp("\\A(?:" + pattern + ")\\z");

    for (auto &line : QString(f.readAll()).split('\n', CUTTER_QT_SKIP_EMPTY_PARTS)) {
        line.replace("#~", "ec ");
        if (!line.isEmpty() && !regexp.match(line).hasMatch()) {
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

const QStringList &ColorThemeWorker::getRizinSpecificOptions()
{
    if (rizinSpecificOptions.isEmpty()) {
        rizinSpecificOptions << Core()->getThemeKeys();
    }
    return rizinSpecificOptions;
}
