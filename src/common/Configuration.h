#ifndef CONFIGURATION_H
#define CONFIGURATION_H

#include <QSettings>
#include <QFont>
#include <core/Cutter.h>

#define Config() (Configuration::instance())
#define ConfigColor(x) Config()->getColor(x)

#ifdef CUTTER_ENABLE_KSYNTAXHIGHLIGHTING
namespace KSyntaxHighlighting {
class Repository;
class Theme;
}
#endif

class QSyntaxHighlighter;
class QTextDocument;

enum ColorFlags {
    LightFlag = 1,
    DarkFlag = 2,
    DualColor = LightFlag | DarkFlag,
};

struct CutterInterfaceTheme
{
    QString name;
    ColorFlags flag;
};

struct RecentFileEntry
{
    QString ioMode;
    QString path;

    bool operator==(const RecentFileEntry &other) const
    {
        return ioMode == other.ioMode && path == other.path;
    }
};

class CUTTER_EXPORT Configuration : public QObject
{
    Q_OBJECT
private:
    QPalette nativePalette;
    QSettings s;
    static Configuration *mPtr;

#ifdef CUTTER_ENABLE_KSYNTAXHIGHLIGHTING
    KSyntaxHighlighting::Repository *kSyntaxHighlightingRepository;
#endif
    bool outputRedirectEnabled = true;

    Configuration();
    // Colors
    void loadBaseThemeNative();
    void loadBaseThemeDark();
    void loadNativeStylesheet();
    void loadLightStylesheet();
    void loadDarkStylesheet();
    void loadMidnightStylesheet();

    // Asm Options
    void applySavedAsmOptions();

public:
    static const QList<CutterInterfaceTheme> &cutterInterfaceThemesList();
    static const QHash<QString, ColorFlags> relevantThemes;
    static const QHash<QString, QHash<ColorFlags, QColor>> cutterOptionColors;

    // Functions
    static Configuration *instance();

    void loadInitial();

    void resetAll();

    // Auto update
    bool getAutoUpdateEnabled() const;
    void setAutoUpdateEnabled(bool au);

    // Languages
    QLocale getCurrLocale() const;
    void setLocale(const QLocale &l);
    bool setLocaleByName(const QString &language);
    struct LangInfo
    {
        QString name;
        QLocale locale;
    };
    std::vector<LangInfo> getAvailableTranslations();

    // Fonts

    /**
     * @brief Gets the configured font set by the font selection box
     * @return the configured font
     */
    const QFont getBaseFont() const;

    /**
     * @brief Gets the configured font with the point size adjusted by the configured zoom
     * level (minimum of 10%)
     * @return the configured font size adjusted by zoom level
     */
    const QFont getFont() const;
    void setFont(const QFont &font);
    qreal getZoomFactor() const;
    void setZoomFactor(qreal zoom);

    // Colors
    bool windowColorIsDark();
    static bool nativeWindowIsDark();
    void setLastThemeOf(const CutterInterfaceTheme &currInterfaceTheme, const QString &theme);
    QString getLastThemeOf(const CutterInterfaceTheme &currInterfaceTheme) const;
    void setInterfaceTheme(int theme);
    int getInterfaceTheme() { return s.value("ColorPalette", 0).toInt(); }

    const CutterInterfaceTheme *getCurrentTheme();

#ifdef CUTTER_ENABLE_KSYNTAXHIGHLIGHTING
    KSyntaxHighlighting::Repository *getKSyntaxHighlightingRepository();
    KSyntaxHighlighting::Theme getKSyntaxHighlightingTheme();
#endif
    QSyntaxHighlighter *createSyntaxHighlighter(QTextDocument *document);

    QString getRecentFolder();
    void setRecentFolder(const QString &dir);

    void setNewFileLastClicked(int lastClicked);
    int getNewFileLastClicked();

    // Images
    QString getLogoFile();

    // Asm Options
    void resetToDefaultAsmOptions();

    QString getColorTheme() const { return s.value("theme", "cutter").toString(); }
    void setColorTheme(const QString &theme);
    /**
     * @brief Change current color theme if it doesn't much native theme's darkness.
     */
    void adjustColorThemeDarkness();
    int colorThemeDarkness(const QString &colorTheme) const;

    void setColor(const QString &name, const QColor &color);
    const QColor getColor(const QString &name) const;

    /**
     * @brief Get the value of a config var either from Rizin or settings, depending on the key.
     */
    QVariant getConfigVar(const QString &key);
    bool getConfigBool(const QString &key);
    int getConfigInt(const QString &key);
    QString getConfigString(const QString &key);

    /**
     * @brief Set the value of a config var either to Rizin or settings, depending on the key.
     */
    void setConfig(const QString &key, const QVariant &value);
    bool isFirstExecution();

    /**
     * @return id of the last selected decompiler (see CutterCore::getDecompilerById)
     */
    QString getSelectedDecompiler();
    void setSelectedDecompiler(const QString &id);

    bool getDecompilerAutoRefreshEnabled();
    void setDecompilerAutoRefreshEnabled(bool enabled);

    void enableDecompilerAnnotationHighlighter(bool useDecompilerHighlighter);
    bool isDecompilerAnnotationHighlighterEnabled();

    // Graph
    int getGraphBlockMaxChars() const { return s.value("graph.maxcols", 100).toInt(); }
    void setGraphBlockMaxChars(int ch) { s.setValue("graph.maxcols", ch); }

    int getGraphMinFontSize() const { return s.value("graph.minfontsize", 4).toInt(); }

    void setGraphMinFontSize(int sz) { s.setValue("graph.minfontsize", sz); }

    /**
     * @brief Get the boolean setting for preview in Graph
     * @return True if preview checkbox is checked, false otherwise
     */
    bool getGraphPreview() { return s.value("graph.preview").toBool(); }
    /**
     * @brief Set the boolean setting for preview in Graph
     * @param checked is a boolean that represents the preview checkbox
     */
    void setGraphPreview(bool checked) { s.setValue("graph.preview", checked); }

    /**
     * @brief Getters and setters for the transaparent option state and scale factor for bitmap
     * graph exports.
     */
    bool getBitmapTransparentState();
    double getBitmapExportScaleFactor();
    void setBitmapTransparentState(bool inputValueGraph);
    void setBitmapExportScaleFactor(double inputValueGraph);
    void setGraphSpacing(QPoint blockSpacing, QPoint edgeSpacing);
    QPoint getGraphBlockSpacing();
    QPoint getGraphEdgeSpacing();

    /**
     * @brief Gets whether the entry offset of each block has to be displayed or not
     * @return true if the entry offset has to be displayed, false otherwise
     */
    bool getGraphBlockEntryOffset();

    /**
     * @brief Enable or disable the displaying of the entry offset in each graph block
     * @param enabled set this to true for displaying the entry offset in each graph block, false
     * otherwise
     */
    void setGraphBlockEntryOffset(bool enabled);

    /**
     * @brief Enable or disable Cutter output redirection.
     * Output redirection state can only be changed early during Cutter initialization.
     * Changing it later will have no effect
     * @param enabled set this to false for disabling output redirection
     */
    void setOutputRedirectionEnabled(bool enabled);
    bool getOutputRedirectionEnabled() const;

    void setPreviewValue(bool checked);
    bool getPreviewValue() const;

    /**
     * @brief Show tooltips for known values of registers, variables, and memory when debugging
     */
    void setShowVarTooltips(bool enabled);
    bool getShowVarTooltips() const;

    /**
     * @brief Recently opened binaries, as shown in NewFileDialog.
     */
    QList<RecentFileEntry> getRecentFiles() const;
    void setRecentFiles(const QList<RecentFileEntry> &list);

    /**
     * @brief Recently opened projects, as shown in NewFileDialog.
     */
    QList<RecentFileEntry> getRecentProjects() const;
    void setRecentProjects(const QList<RecentFileEntry> &list);
    void addRecentProject(QString file);

    // Functions Widget Layout

    /**
     * @brief Get the layout of the Functions widget.
     * @return The layout.
     */
    QString getFunctionsWidgetLayout();

    /**
     * @brief Set the layout of the Functions widget
     * @param layout The layout of the Functions widget, either horizontal or vertical.
     */
    void setFunctionsWidgetLayout(const QString &layout);
public slots:
    void refreshFont();
signals:
    void fontsUpdated();
    void colorsUpdated();
    void interfaceThemeChanged();
#ifdef CUTTER_ENABLE_KSYNTAXHIGHLIGHTING
    void kSyntaxHighlightingThemeChanged();
#endif
};

#endif // CONFIGURATION_H
