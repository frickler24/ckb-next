#ifndef DEBUG_H
#define DEBUG_H

#include <qdebug.h>

/// \brief DEBUGLEVEL_CONFIGURED is the initial value for debugLebel and is used in main.cpp
#define DEBUGLEVEL_CONFIGURED   10

///\brief debugLevel controls the amount of debug output.
///     0 = no debug output, just hard errors are displayed
///    10 = full debug output with complete logging
static u_int8_t debugLevel;


#endif // DEBUG_H
