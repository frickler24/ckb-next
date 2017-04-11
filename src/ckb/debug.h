#ifndef DEBUG_H
#define DEBUG_H

#include <qdebug.h>
#include <QDebug>

/// \brief DEBUGLEVEL_CONFIGURED is the initial value for debugLebel and is used in main.cpp
#define DEBUGLEVEL_CONFIGURED 10
#define DEBUGLEVEL_MAX        DEBUGLEVEL_CONFIGURED

///\brief debugLevel controls the amount of debug output.

///     0 = no debug output, just hard errors are displayed
///     5 = Error-handling and inconsistencies, but less than info
///     8 = All info, but no runtime data (eg logging from usb)
///    10 = full debug output with complete logging
#define DEBUGLEVEL_HARDERR  0
#define DEBUGLEVEL_INCONSISTENT 5
#define DEBUGLEVEL_INFO 8
#define DEBUGLEVEL_ALL 10

class Debug {

public:
    Debug();
    Debug(u_int8_t newLevel);
    void setDebugLevel(u_int8_t newLevel);
    u_int8_t getDebugLevel();

private:
    u_int8_t debugLevel;
};

#endif // DEBUG_H
