
#ifndef SETTINGS_H
#define SETTINGS_H

#include <QSettings>

class Settings
{
private:
    QSettings settings;

public:
    bool getAsmBytes() const            { return settings.value("bytes", false).toBool(); }
    void setAsmBytes(bool v)            { settings.setValue("bytes", v); }

    bool getATnTSyntax() const          { return settings.value("syntax", false).toBool(); }
    void setATnTSyntax(bool v)          { settings.setValue("syntax", v); }

    bool getOpcodeDescription() const   { return settings.value("describe", false).toBool(); }
    void setOpcodeDescription(bool v)   { settings.setValue("describe", v); }

    bool getStackPointer() const        { return settings.value("stackptr", false).toBool(); }
    void setStackPointer(bool v)        { settings.setValue("stackptr", v); }

    bool getUppercaseDisas() const      { return settings.value("ucase", false).toBool(); }
    void setUppercaseDisas(bool v)      { settings.setValue("ucase", v); }

    bool getSpacy() const               { return settings.value("spacy", false).toBool(); }
    void setSpacy(bool v)               { settings.setValue("spacy", v); }
};

#endif // SETTINGS_H
