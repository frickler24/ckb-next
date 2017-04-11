#include "debug.h"

///
/// \brief Debug::Debug Defines the level of debug output to generate or to suppress
/// \param newLevel
///
Debug::Debug() {
    setDebugLevel(DEBUGLEVEL_CONFIGURED);
    QDebug debugger = qDebug();

    debugger.setVerbosity(0);
    debugger << "Verbosity =" << debugger.verbosity();
}

Debug::Debug(u_int8_t newLevel) {
    setDebugLevel(newLevel);
}

///
/// \brief Debug::setDebugLevel (setter)
/// \param newLevel sets the new level
void Debug::setDebugLevel(u_int8_t newLevel) {
    if (newLevel > DEBUGLEVEL_MAX) {
        if (debugLevel > DEBUGLEVEL_INFO) qDebug() << "Trying to set debugLevel too high:"
            << newLevel << "requested," << DEBUGLEVEL_MAX << "is maximum allowed.";
    } else {
        debugLevel = newLevel;
    }
}

///
/// \brief Debug::getDebugLevel (getter)
/// \return u_int8_t debugLevel
u_int8_t Debug::getDebugLevel() {
    return debugLevel;
}
