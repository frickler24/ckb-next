#ifndef DEBUG_H
#define DEBUG_H
#include <QDebug>

/// \brief DEBUGLEVEL_CONFIGURED is the initial value for debugLebel and is used in main.cpp
#define DEBUGLEVEL_INITIAL 2

/// \brief debugLevel controls the amount of debug output.
///     0 = no debug output, just hard errors (Fatal) are displayed in a short form
///     1 = no debug output, just hard errors (Fatal) are displayed in a long form
///     2 = Error-handling and inconsistencies (Fatal + Critical = default value) in short form
///     3 = as 2 but in long form
///     4 = as 2 + Warnings (in short form)
///     5 = as 4 but in long form
///     6 = All info, but no debug data (eg logging from usb) in short form
///     7 = full debug output with complete logging runtime data (eg logging from usb) in long form
#define DEBUGLEVEL_FATAL  0
#define DEBUGLEVEL_FATAL_LONG 1
#define DEBUGLEVEL_CRITICAL 2
#define DEBUGLEVEL_CRITICAL_LONG 3
#define DEBUGLEVEL_WARNING 4
#define DEBUGLEVEL_WARNING_LONG 5
#define DEBUGLEVEL_INFO 6
#define DEBUGLEVEL_ALL 7

#define DEBUGLEVEL_MAX DEBUGLEVEL_ALL

#define DEBUGBUFFERSIZE 1024
#endif // DEBUG_H
