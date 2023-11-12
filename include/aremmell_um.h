#ifndef _AREMMELL_UM_H_INCLUDED
# define _AREMMELL_UM_H_INCLUDED

#include <UMS3.h>

namespace aremmell
{
    /// @brief Alternates the RGB pixel between red and off at the specified
    /// interval. Indicates a fatal problem to the operator with an unmistakable
    /// visual cue.
    /// @param ums3 Reference to the UMS3 instance to use.
    /// @param interval Alternation interval in milliseconds.
    static inline
    void on_fatal_error(UMS3& ums3, uint32_t interval = 1000U)
    {
        ums3.setPixelPower(true); // assume pixel could be off.
        ums3.setPixelBrightness(255); // maximum brightness.

        while (true) {
            ums3.setPixelColor(0xff, 0x00, 0x00); // pure red.
            delay(interval);
            ums3.setPixelColor(0x00, 0x00, 0x00); // black (off).
            delay(interval);
        }
    }
} // namespace aremmell


#endif // !_AREMMELL_UM_H_INCLUDED
