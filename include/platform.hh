#ifndef _TWM_PLATFORM_H_INCLUDED
#define _TWM_PLATFORM_H_INCLUDED

#include <cstdlib>
#include <cstdint>
#include <cinttypes>

namespace twm
{
    /** Library-level error code. */
    using ErrorCode = uint8_t;

    /** Coordinate (e.g. X, Y, Z). */
    using Coord = int16_t;

    /** Extent (e.g. width, height). */
    using Extent = uint16_t;
} // namespace twm

#endif // !_TWM_PLATFORM_H_INCLUDED
