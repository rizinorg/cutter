#ifndef CUTTER_H
#define CUTTER_H

#include "sessions/DynamicSession.h"

#define Core() (LegacyLock { DynamicSession::instance() })
#define Signal() DynamicSession::instance()->getWrapper()
#define Session() DynamicSession::instance()

#endif // CUTTER_H