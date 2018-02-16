#ifndef CONFIGURATION_H
#define CONFIGURATION_H

#include <QSettings>
#include <QFont>
#include <cutter.h>

#define Config() (Configuration::instance())
#define ConfigColor(x) Config()->getColor(x)

class Configuration : public QObject
{
    Q_OBJECT
private:
    QSettings s;
    static Configuration* mPtr;

    void loadInitial();

    // Colors
    void loadDefaultTheme();
    void loadDarkTheme();
    void setColor(const QString &name, const QColor &color);

    // Images
    QString logoFile;

public:
    // Functions
    Configuration();
    static Configuration* instance();

    void resetAll();

    // Fonts
    const QFont getFont() const;
    void setFont(const QFont &font);

    // Colors
    const QColor getColor(const QString &name) const;
    void setDarkTheme(bool set);
    bool getDarkTheme()                 { return s.value("dark").toBool(); }

    // Images
    QString getLogoFile();

    // Graph
    int getGraphBlockMaxChars() const   { return s.value("graph.maxcols", 50).toInt(); }
    void setGraphBlockMaxChars(int ch)  { s.setValue("graph.maxcols", ch); }

    // TODO Imho it's wrong doing it this way. Should find something else.
    bool getAsmESIL() const             { return s.value("asm.esil", false).toBool(); }
    void setAsmESIL(bool v)             { s.setValue("asm.esil", v); }

    bool getAsmPseudo() const           { return s.value("asm.pseudo", false).toBool(); }
    void setAsmPseudo(bool v)           { s.setValue("asm.pseudo", v); }

    bool getAsmOffset() const           { return s.value("asm.offset", true).toBool(); }
    void setAsmOffset(bool v)           { s.setValue("asm.offset", v); }

    bool getAsmDescribe() const         { return s.value("asm.describe", false).toBool(); }
    void setAsmDescribe(bool v)         { s.setValue("asm.describe", v); }

    bool getAsmStackPointer() const     { return s.value("asm.stackptr", false).toBool(); }
    void setAsmStackPointer(bool v)     { s.setValue("asm.stackptr", v); }

    bool getAsmSlow() const             { return s.value("asm.slow", true).toBool(); }
    void setAsmSlow(bool v)             { s.setValue("asm.slow", v); }

    bool getAsmLines() const            { return s.value("asm.lines", true).toBool(); }
    void setAsmLines(bool v)            { s.setValue("asm.lines", v); }

    bool getAsmFcnLines() const         { return s.value("asm.fcnlines", true).toBool(); }
    void setAsmFcnLines(bool v)         { s.setValue("asm.fcnlines", v); }

    bool getAsmEmu() const              { return s.value("asm.emu", false).toBool(); }
    void setAsmEmu(bool v)              { s.setValue("asm.emu", v); }

    bool getAsmCmtRight() const         { return s.value("asm.cmtright", true).toBool(); }
    void setAsmCmtRight(bool v)         { s.setValue("asm.cmtright", v); }

    bool getAsmVarSum() const           { return s.value("asm.varsum", false).toBool(); }
    void setAsmVarSum(bool v)           { s.setValue("asm.varsum", v); }

    bool getAsmBytes() const            { return s.value("asm.bytes", false).toBool(); }
    void setAsmBytes(bool v)            { s.setValue("asm.bytes", v); }

    bool getAsmBytespace() const        { return s.value("asm.bytespace", false).toBool(); }
    void setAsmBytespace(bool v)        { s.setValue("asm.bytespace", v); }

    bool getAsmLBytes() const           { return s.value("asm.lbytes", true).toBool(); }
    void setAsmLBytes(bool v)           { s.setValue("asm.lbytes", v); }

    int getAsmNBytes() const            { return s.value("asm.nbytes", 10).toInt(); }
    void setAsmNBytes(int v)            { s.setValue("asm.nbytes", v); }

    QString getAsmSyntax() const        { return s.value("asm.syntax", "intel").toString(); }
    void setAsmSyntax(const QString &v) { s.setValue("asm.syntax", v); }

    bool getAsmUppercase() const        { return s.value("asm.ucase", false).toBool(); }
    void setAsmUppercase(bool v)        { s.setValue("asm.ucase", v); }

    bool getAsmBBLine() const           { return s.value("asm.bbline", false).toBool(); }
    void setAsmBBLine(bool v)           { s.setValue("asm.bbline", v); }

    bool getAsmCapitalize() const       { return s.value("asm.capitalize", false).toBool(); }
    void setAsmCapitalize(bool v)       { s.setValue("asm.capitalize", v); }

    bool getAsmVarsub() const           { return s.value("asm.varsub", true).toBool(); }
    void setAsmVarsub(bool v)           { s.setValue("asm.varsub", v); }

    bool getAsmVarsubOnly() const       { return s.value("asm.varsub_only", true).toBool(); }
    void setAsmVarsubOnly(bool v)       { s.setValue("asm.varsub_only", v); }

    int getAsmTabs() const              { return s.value("asm.tabs", 5).toInt(); }
    void setAsmTabs(int v)              { s.setValue("asm.tabs", v); }

signals:
    void fontsUpdated();
    void colorsUpdated();
};

#endif // CONFIGURATION_H
