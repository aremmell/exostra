/*
 * gfx_driver.hh
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
#ifndef _TWM_GFX_DRIVER_H_INCLUDED
#define _TWM_GFX_DRIVER_H_INCLUDED

#include <primitives.hh>

namespace twm
{
    /** Abstract low-level graphics driver interface. */
    class GfxDriver
    {
    public:
        GfxDriver() = default;
        virtual ~GfxDriver() = default;

        virtual bool init() = 0;
        virtual void tearDown() = 0;

        virtual void beginTransaction() = 0;
        virtual void endTransaction() = 0;

        virtual void setCursor(const Point& cursor) = 0;
        virtual Coord getCursorX() const = 0;
        virtual Coord getCursorY() const = 0;

        virtual void fillDisplay(Color color) = 0;

        virtual void writePixel(const Point& point, Color color) = 0;
        virtual void writeLine(const Point& start, const Point& end, Color color) = 0;
        virtual void writeFilledRect(const Rect& rect, Color color) = 0;

        virtual void drawPixel(const Point& point, Color color) = 0;
        virtual void drawLine(const Point& start, const Point& end, Color color) = 0;
        virtual void drawRect(const Rect& rect, uint8_t thickness, Color color) = 0;
        virtual void drawFilledRect(const Rect& rect, Color color) = 0;
        virtual void drawCircle(const Point& middle, Extent radius, Color color) = 0;

        // TODO: fonts:
        // need the concept of a device context/canvas that can have properties
        // like background color, border color, font, font color, etc.
        // each 'window' should "be one" of these

        // TODO: text w/ bitmask for wrapping, ellipsis, etc.

        // TODO: bitmaps
    };
} // namespace twm

#endif // !_TWM_GFX_DRIVER_H_INCLUDED
