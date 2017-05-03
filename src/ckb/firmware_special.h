#ifndef FIRMWARE_SPECIAL_H
#define FIRMWARE_SPECIAL_H
#include "keymap.h"

/// \file Define vendor:product combinations, whicxh need special treatment in firmware detection.
/// This may be neccessary, if a new hardware uses the same features as an older hardware,
/// but  needs a completely different firmware-file or -version.

/// To reduce effort, please insert only these combinations, whcih need really special treatment.
/// All other models will further be identified by their featrur-lists.
///
/// And - please - add a comment, why this antry is needed.

/// Don't change this. If Corsair is not corsair any more, we have a different problem...
#define V_CORSAIR       0x1b1c
#define P_K70_LUX_NRGB	     0x1b36

struct firmwareSpecial {
    const short product;
    const KeyMap::Model model;
} specials[] = {

    /// ckb-next Issue #97:
    /// client tries to install a firmware for ProductID 0x1b13 (K70) into the device with productID 0x1b36 (K70_LUX_NRGB).
    /// Reason is, that 0x1b36 is non-RGB, but hast Lightning.
    /// cat features: corsair k70 rgb pollrate bind notify fwversion fwupdate
    /// cat model: Corsair Gaming K70 LUX Keyboard
    ///
    /// Because it detects "rgb", the standard choice is K70. We have to it to K70_LUX (0x1b33).
    { P_K70_LUX_NRGB, KeyMap::K70},
};

#endif // FIRMWARE_SPECIAL_H