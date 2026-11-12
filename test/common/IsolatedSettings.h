#ifndef CUTTER_TEST_ISOLATED_SETTINGS_H
#define CUTTER_TEST_ISOLATED_SETTINGS_H

#include <QApplication>
#include <QCoreApplication>
#include <QSettings>
#include <QTemporaryDir>
#include <QTest>

namespace CutterTest {

/**
 * @brief Points QSettings at an empty temporary folder for this process.
 *
 * Cutter's Configuration shows a modal "Settings are not writable" box when QSettings can't
 * write, which happens on Windows without an organization name. Isolated settings also keep a
 * test from reading or changing the user's own Cutter configuration.
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

/**
 * @brief QTEST_MAIN with isolated settings: a plain QApplication, no core and no window.
 */
#define CUTTER_UNIT_TEST_MAIN(TestClass)                                                           \
    int main(int argc, char *argv[])                                                               \
    {                                                                                              \
        CutterTest::isolateSettings();                                                             \
        QApplication app(argc, argv);                                                              \
        TestClass test;                                                                            \
        return QTest::qExec(&test, argc, argv);                                                    \
    }

#endif // CUTTER_TEST_ISOLATED_SETTINGS_H
