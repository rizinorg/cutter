
#ifndef SETTINGS_H
#define SETTINGS_H

#include <QSettings>

class Settings
{
private:
    QSettings settings;

public:
    bool getAsmESIL() const             { return settings.value("asm.esil", false).toBool(); }
    void setAsmESIL(bool v)             { settings.setValue("asm.esil", v); }

    bool getAsmPseudo() const           { return settings.value("asm.pseudo", false).toBool(); }
    void setAsmPseudo(bool v)           { settings.setValue("asm.pseudo", v); }

    bool getAsmOffset() const           { return settings.value("asm.offset", true).toBool(); }
    void setAsmOffset(bool v)           { settings.setValue("asm.offset", v); }

    bool getAsmDescribe() const         { return settings.value("asm.describe", false).toBool(); }
    void setAsmDescribe(bool v)         { settings.setValue("asm.describe", v); }

    bool getAsmStackPointer() const     { return settings.value("asm.stackptr", false).toBool(); }
    void setAsmStackPointer(bool v)     { settings.setValue("asm.stackptr", v); }

    bool getAsmBytes() const            { return settings.value("asm.bytes", false).toBool(); }
    void setAsmBytes(bool v)            { settings.setValue("asm.bytes", v); }

    bool getAsmBytespace() const        { return settings.value("asm.bytespace", false).toBool(); }
    void setAsmBytespace(bool v)        { settings.setValue("asm.bytespace", v); }

    bool getAsmLBytes() const           { return settings.value("asm.lbytes", true).toBool(); }
    void setAsmLBytes(bool v)           { settings.setValue("asm.lbytes", v); }

    QString getAsmSyntax() const        { return settings.value("asm.syntax", "intel").toString(); }
    void setAsmSyntax(const QString &v) { settings.setValue("asm.syntax", v); }

    bool getAsmUppercase() const        { return settings.value("asm.ucase", false).toBool(); }
    void setAsmUppercase(bool v)        { settings.setValue("asm.ucase", v); }

    bool getAsmBBLine() const           { return settings.value("asm.bbline", false).toBool(); }
    void setAsmBBLine(bool v)           { settings.setValue("asm.bbline", v); }

    bool getAsmCapitalize() const       { return settings.value("asm.capitalize", false).toBool(); }
    void setAsmCapitalize(bool v)       { settings.setValue("asm.capitalize", v); }

    bool getAsmVarsub() const           { return settings.value("asm.varsub", true).toBool(); }
    void setAsmVarsub(bool v)           { settings.setValue("asm.varsub", v); }

    bool getAsmVarsubOnly() const       { return settings.value("asm.varsub_only", true).toBool(); }
    void setAsmVarsubOnly(bool v)       { settings.setValue("asm.varsub_only", v); }
};

#endif // SETTINGS_H
