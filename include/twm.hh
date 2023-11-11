/*
 * twm.hh
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
#ifndef _TWM_H_INCLUDED
# define _TWM_H_INCLUDED

# include <cstdlib>
# include <cstdint>
# include <cinttypes>
# include <type_traits>
# include <memory>
# include <mutex>

# include <Adafruit_GFX.h>

// TODO: remove me
#define TWM_COLOR_565

#define TWM_ASSERT(expr) \
    do { \
        if (!(expr)) { \
            Serial.printf("error: assert!!! (" #expr ") at %s:%d\n", \
                __FILE__, __LINE__); \
        } \
    } while (false);

namespace twm
{
    /** For now, the only low-level graphics interface supported is Adafruit's. */
    using RawGfxInterface = Adafruit_GFX;

    /** Use the smallest type that can contain all possible colors. */
# if defined(TWM_COLOR_MONOCHROME) || defined(TWM_COLOR_256)
    using Color = uint8_t; /** Color (monochrome/8-bit). */
# elif defined(TWM_COLOR_565)
    using Color = uint16_t; /** Color (16-bit 565 RGB) */
# elif defined(TWM_COLOR_RGB) || defined(TWM_COLOR_ARGB)
    using Color = uint32_t; /** Color (24-bit RGB/32-bit ARGB). */
# else
#  error "color mode is invalid"
# endif

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

    template<class TImpl>
    class GfxContext : public TImpl
    {
    public:
        template<typename... TArgs>
        GfxContext(TArgs&& ...args) : TImpl(args...) { }

        virtual ~GfxContext() = default;

        static std::shared_ptr<GfxContext<TImpl>>& getInstance()
        {
            TWM_ASSERT(_instance != nullptr);
            return _instance;
        }

        template<typename... TArgs>
        static std::shared_ptr<GfxContext<TImpl>> init(TArgs&& ...args)
        {
            static std::once_flag flag;
            std::call_once(flag, [&]()
            {
                _instance.reset(new GfxContext<TImpl>(args...));
            });
            TWM_ASSERT(_instance != nullptr);
            return _instance;
        }

        virtual void tearDown() { }

    protected:
        static std::shared_ptr<GfxContext<TImpl>> _instance;
    };

    template<class TImpl>
    std::shared_ptr<GfxContext<TImpl>> GfxContext<TImpl>::_instance;

} // namespace twm


#endif // !_TWM_H_INCLUDED
