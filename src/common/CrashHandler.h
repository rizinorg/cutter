#ifndef CRASH_HANDLER_H
#define CRASH_HANDLER_H

#include <QString>

/**
 * @fn void initCrashHandler()
 *
 * If CUTTER_ENABLE_CRASH_REPORTS is true, initializes
 * crash handling and reporting, otherwise does nothing.
*/
void initCrashHandler();

void showCrashDialog(const QString &dumpFile);

#endif // CRASH_HANDLER_H
