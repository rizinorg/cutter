#ifndef CRASH_HANDLER_H
#define CRASH_HANDLER_H

/**
 * @fn void initCrashHandler()
 *
 * Initializes crash handling and reporting. If CUTTER_ENABLE_CRASH_REPORTS is true
 * also initializes crash dump system.
*/
void initCrashHandler();

#endif // CRASH_HANDLER_H
