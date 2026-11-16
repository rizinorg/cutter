#ifndef CUTTER_TEST_ISOLATED_SETTINGS_H
#define CUTTER_TEST_ISOLATED_SETTINGS_H

#include <QApplication>
#include <QCoreApplication>
#include <QSettings>
#include <QTemporaryDir>
#include <QTest>

namespace CutterTest {

/**
 * @brief Points QSettings to an empty temporary folder for this process.
 */
inline void isolateSettings()
{
    static QTemporaryDir settingsDir;
    QCoreApplication::setOrganizationName("rizin");
    QCoreApplication::setApplicationName("cutter-tests");
    QSettings::setDefaultFormat(QSettings::IniFormat);
    QSettings::setPath(QSettings::IniFormat, QSettings::UserScope, settingsDir.path());
}

} // namespace CutterTest

#endif // CUTTER_TEST_ISOLATED_SETTINGS_H
