/*
 * primitives.hh
 *
 * Author:    Ryan M. Lederman <lederman@gmail.com>
 * Copyright: Copyright (c) 2023
 * Version:   0.0.1
 * License:   The MIT License (MIT)
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of
 * this software and associated documentation files (the "Software"), to deal in
 * the Software without restriction, including without limitation the rights to
 * use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
 * the Software, and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */
#ifndef _TWM_PRIMITIVES_H_INCLUDED
# define _TWM_PRIMITIVES_H_INCLUDED

#include <platform.hh>

// TODO: Some of this functionality dealing with coordinates will
// depend on where 0,0 lies and which direction X and Y go. For now,
// I'm only going to implement what makes sense to my brain: 0,0 is
// the top left corner, X increases to the right, and Y increases
// downward.

namespace twm
{
    /** Coordinate in 3D space (e.g. X, Y, or Z). */
    using Coord = int16_t;

    /** Extent (e.g. width, height). */
    using Extent = uint16_t;

    /** Point in 2D space. */
    struct Point
    {
        Coord x; /** X-axis value. */
        Coord y; /** Y-axis value. */
    };

    /** Two points in 2D space (left/top, right/bottom). */
    struct Rect
    {
        Coord left {};    /** X-axis value of the left edge. */
        Coord top {};     /** Y-axis value of the top edge. */
        Coord right {};   /** X-axis value of the right edge. */
        Coord bottom {};  /** Y-axis value of the bottom edge. */

        inline Extent width() const noexcept
        {
            TWM_ASSERT(right >= left);
            return static_cast<Extent>(right - left);
        }

        inline Extent height() const noexcept
        {
            TWM_ASSERT(bottom >= top);
            return static_cast<Extent>(bottom - top);
        }

        inline Point getTopLeft() const noexcept
        {
            return { left, top };
        }

        inline Point getBottomRight() const noexcept
        {
            return { right, bottom };
        }

        inline void grow(Extent px) noexcept
        {
            left   -= px;
            top    -= px;
            right  += px;
            bottom += px;
        }

        inline void shrink(Extent px) noexcept
        {
            left   += px;
            top    += px;
            right  -= px;
            bottom -= px;
        }

        inline bool isSquare() const noexcept
        {
            return width() == height();
        }

        inline bool isHorizontal() const noexcept
        {
            return width() > height();
        }

        inline bool isVertical() const noexcept
        {
            return height() > width();
        }
    };

} // namespace twm


#endif // !_TWM_PRIMITIVES_H_INCLUDED
