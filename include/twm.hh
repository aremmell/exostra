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
# include <functional>
# include <string>
# include <memory>
# include <queue>
# include <vector>
# include <mutex>
# include <map>

# include <Adafruit_GFX.h>
# include <Fonts/FreeSans9pt7b.h>
# include <Fonts/FreeSans12pt7b.h>
# include <Fonts/FreeSans18pt7b.h>
# include <glcdfont.c>

# if !defined(_ARDUINO_GFX_H_)
# if defined(__AVR__)
#  include <avr/pgmspace.h>
# elif defined(ESP8266) || defined(ESP32)
#  include <pgmspace.h>
# endif

# if !defined(pgm_read_byte)
#  define pgm_read_byte(addr) (*(reinterpret_cast<const uint8_t*>(addr)))
# endif
# if !defined(pgm_read_word)
#  define pgm_read_word(addr) (*(reinterpret_cast<const uint16_t*>(addr)))
# endif
# if !defined(pgm_read_dword)
#  define pgm_read_dword(addr) (*(reinterpret_cast<const uint32_t*>(addr)))
# endif

# if !defined(pgm_read_pointer)
#  if !defined(__INT_MAX__) || (__INT_MAX__ > 0xffff)
#   define pgm_read_pointer(addr) (static_cast<void*>(pgm_read_dword(addr)))
#  else
#   define pgm_read_pointer(addr) (static_cast<void*>(pgm_read_word(addr)))
#  endif
# endif

inline GFXglyph* pgm_read_glyph_ptr(const GFXfont* gfxFont, uint8_t c)
{
# ifdef __AVR__
    return &((static_cast<GFXglyph*>(pgm_read_pointer(&gfxFont->glyph)))[c]);
# else
    return gfxFont->glyph + c;
# endif
}
# endif // !_ARDUINO_GFX_H_

static void AdafruitExt_getCharBounds(uint8_t ch, uint8_t* cx, uint8_t* cy,
    uint8_t* xAdv, uint8_t* yAdv, int8_t* xOff, int8_t* yOff, uint8_t textSizeX = 1,
    uint8_t textSizeY = 1, const GFXfont* gfxFont = nullptr)
{
    if (gfxFont) { // Next line: x = start; y += textSizeY * yAdv;
        uint8_t first = pgm_read_byte(&gfxFont->first),
                last  = pgm_read_byte(&gfxFont->last);
        bool inRange = (ch >= first && ch <= last);
        GFXglyph* glyph = inRange ? pgm_read_glyph_ptr(gfxFont, ch - first) : nullptr;
        if (cx) {
            *cx = inRange ? pgm_read_byte(&glyph->width) : 0;
        }
        if (cy) {
            *cy = inRange ? pgm_read_byte(&glyph->height) : 0;
        }
        *xAdv = inRange ? pgm_read_byte(&glyph->xAdvance) : 0;
        *yAdv = inRange ? pgm_read_byte(&gfxFont->yAdvance) : 0;
        *xOff = inRange ? pgm_read_byte(&glyph->xOffset) : 0;
        *yOff = inRange ? pgm_read_byte(&glyph->yOffset) : 0;
    } else { // Next line: x = start; y += textSizeY * 8;
        *cx = textSizeX * 6;
        *cy = textSizeY * 8;
        *xAdv = *cx;
        *yAdv = *cy;
        *xOff = 0;
        *yOff = 0;
    }
}

// TODO: remove me
# define TWM_COLOR_565

// Uncomment to remove mutex locks.
//#define TWM_SINGLETHREAD

// Comment out to disable logging.
# define TWM_ENABLE_LOGGING

# if defined(TWM_ENABLE_LOGGING)
    typedef enum
    {
        TWM_ERROR = 1,
        TWM_WARN  = 2,
        TWM_DEBUG = 3
    } _LoggingLevels;

#  define _TWM_STRIFY(val) #val
#  define TWM_STRIFY(val) _TWM_STRIFY(val)

#  define TWM_LOG(lvl, fmt, ...) \
    do { \
        char prefix = '\0'; \
        switch(lvl) { \
            case TWM_ERROR: prefix = 'E'; break; \
            case TWM_WARN: prefix  = 'W'; break; \
            case TWM_DEBUG: prefix = 'D'; break; \
        } \
        Serial.printf("[%c] (", prefix); \
        Serial.printf(__FILE__ ":" TWM_STRIFY(__LINE__) "): " fmt "\n" \
            __VA_OPT__(,) __VA_ARGS__); \
    } while(false)
# else
#  define TWM_LOG(lvl, fmt, ...)
# endif

# define TWM_ASSERT(expr) \
    do { \
        if (!(expr)) { \
            TWM_LOG(TWM_ERROR, "assert: '" #expr "'"); \
        } \
    } while (false)

# define TWM_CONST(type, name, value) \
    static constexpr PROGMEM type name = value;

namespace twm
{
    /** For now, the only low-level graphics interface supported is Adafruit's. */
    using IGfxDriver = GFXcanvas16;
    using GfxDriverPtr = std::shared_ptr<IGfxDriver>;

    /** Window identifier. */
    using WindowID = uint8_t;

    /** Reserved window identifiers. */
    TWM_CONST(WindowID, WID_INVALID,  -1);
    TWM_CONST(WindowID, WID_DESKTOP,   0);
    TWM_CONST(WindowID, WID_PROMPT,    1);
    TWM_CONST(WindowID, WID_PROMPTLBL, 2);

    /** Window style. */
    using Style = uint16_t;

    /** Window state. */
    using State = uint16_t;

    /** Window message parameter type. */
    using MsgParam = uint64_t;

    /** Window message parameter sub-type. */
    using MsgParamWord = uint32_t;

    /** Use the smallest type that can contain all possible colors. */
# if defined(TWM_COLOR_MONOCHROME) || defined(TWM_COLOR_256)
    using Color = uint8_t; /** Color (monochrome/8-bit). */
# elif defined(TWM_COLOR_565)
    using Color = uint16_t; /** Color (16-bit RGB) */
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
        Point(Coord _x, Coord _y) : x(_x), y(_y) { }

        Coord x = 0; /** X-axis value. */
        Coord y = 0; /** Y-axis value. */
    };

    /** Two points in 2D space (left/top, right/bottom). */
    struct Rect
    {
        Rect() = default;

        Rect(Coord l, Coord t, Extent r, Extent b)
            : left(l), top(t), right(r), bottom(b)
        {
        }

        Coord left   = 0; /** X-axis value of the left edge. */
        Coord top    = 0; /** Y-axis value of the top edge. */
        Coord right  = 0; /** X-axis value of the right edge. */
        Coord bottom = 0; /** Y-axis value of the bottom edge. */

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

        inline void inflate(Extent px) noexcept
        {
            left   -= px;
            top    -= px;
            right  += px;
            bottom += px;
        }

        inline void deflate(Extent px) noexcept
        {
            left   += px;
            top    += px;
            right  -= px;
            bottom -= px;
        }

        inline bool overlaps(const Rect& other) const noexcept
        {
            if ((left >= other.left && left <= other.right) ||
                (right <= other.right && right >= other.left)) {
                if ((top < other.top && top + height() >= other.top) ||
                    (bottom > other.bottom && bottom - height() <= other.bottom) ||
                    (top >= other.top && bottom <= other.bottom)) {
                    return true;
                }
            }
            if ((top >= other.top && top <= other.bottom) ||
                (bottom <= other.bottom && bottom >= other.top)) {
                if ((left < other.left && left + width() >= other.left) ||
                    (right > other.right && right - width() <= other.right) ||
                    (left >= other.left && right <= other.right)) {
                    return true;
                }
            }

            return false;
        }

        inline bool isPointWithin(Coord x, Coord y) const noexcept
        {
            if (x >= left && x <= left + width() &&
                y >= top  && y <= top + height()) {
                return true;
            }
            return false;
        }

        inline bool isPointWithin(const Point& point) const noexcept
        {
            return isPointWithin(point.x, point.y);
        }
    };

    template<typename T1, typename T2>
    inline bool bitsHigh(const T1& bitmask, const T2& bits)
    {
        return (bitmask & bits) == bits;
    }

    using Mutex     = std::recursive_mutex;
    using ScopeLock = std::lock_guard<Mutex>;

    typedef enum
    {
        MSG_NONE    = 0,
        MSG_CREATE  = 1,
        MSG_DESTROY = 2,
        MSG_DRAW    = 3,
        MSG_INPUT   = 4,
        MSG_EVENT   = 5,
        MSG_RESIZE  = 6
    } Message;

    typedef enum
    {
        STY_VISIBLE   = 1 << 0,
        STY_CHILD     = 1 << 1,
        STY_AUTOSIZE  = 1 << 2,
        STY_BUTTON    = 1 << 3,
        STY_LABEL     = 1 << 4,
        STY_PROMPT    = 1 << 5,
        STY_PROGBAR   = 1 << 6
    } _WindowStyleFlags;

    typedef enum
    {
        PBR_NORMAL        = 1 << 0,
        PBR_INDETERMINATE = 1 << 1
    } _ProgressBarStyleFlags;

    typedef enum
    {
        STA_ALIVE = 1 << 0
    } _WindowStateFlags;

    typedef enum
    {
        DTF_CENTER = 0x01, /**< Horizontal align center. */
        DTF_SINGLE = 0x02 /**< Single line of text. */
    } _DrawTextFlags;

    typedef enum {
        EVT_CHILD_TAPPED = 1
    } EventType;

    typedef enum {
        INPUT_NONE = 0,
        INPUT_TAP  = 1
    } InputType;

    struct InputParams
    {
        WindowID handledBy = WID_INVALID;
        InputType type = INPUT_NONE;
        Coord x = 0;
        Coord y = 0;
    };

    inline MsgParam makeMsgParam(MsgParamWord hiWord, MsgParamWord loWord)
    {
        return (static_cast<MsgParam>(hiWord) << 32) | (loWord & 0xffffffffU);
    }

    inline MsgParamWord getMsgParamHiWord(MsgParam msgParam)
    {
        return ((msgParam >> 32) & 0xffffffffU);
    }

    inline MsgParamWord getMsgParamLoWord(MsgParam msgParam)
    {
        return (msgParam & 0xffffffffU);
    }

    class ITheme
    {
    public:
        enum ScreenSize
        {
            Small = 0,
            Medium,
            Large
        };
        virtual void setGfxDriver(const GfxDriverPtr&) = 0;
        virtual void drawScreensaver() const = 0;
        virtual void drawDesktopBackground() const = 0;
        virtual void setFont(const GFXfont*) const = 0;
        virtual void setSmallFont() const = 0;
        virtual void setMediumFont() const = 0;
        virtual void setLargeFont() const = 0;
        virtual const GFXfont* autoSelectFont() const = 0;
        virtual const GFXfont* getSmallFont() const = 0;
        virtual const GFXfont* getMediumFont() const = 0;
        virtual const GFXfont* getLargeFont() const = 0;
        virtual ScreenSize getScreenSize() const = 0;
        virtual Extent getScaledValue(Extent) const = 0;
        virtual Extent getWindowXPadding() const = 0;
        virtual Extent getWindowYPadding() const = 0;
        virtual Extent getButtonWidth() const = 0;
        virtual Extent getButtonHeight() const = 0;
        virtual uint8_t getButtonTextSize() const = 0;
        virtual Color getButtonTextColor() const = 0;
        virtual Color getButtonTextColorPressed() const = 0;
        virtual Color getButtonBgColor() const = 0;
        virtual Color getButtonBgColorPressed() const = 0;
        virtual Color getButtonFrameColor() const = 0;
        virtual Color getButtonFrameColorPressed() const = 0;
        virtual Extent getButtonLabelPadding() const = 0;
        virtual u_long getButtonTappedDuration() const = 0;
        virtual Coord getButtonCornerRadius() const = 0;
        virtual uint8_t getWindowTextSize() const = 0;
        virtual Color getWindowTextColor() const = 0;
        virtual Extent getWindowFrameThickness() const = 0;
        virtual void drawWindowFrame(const Rect&) const = 0;
        virtual void drawWindowBackground(const Rect&) const = 0;
        virtual void drawText(const char*, uint8_t,
            const Rect&, uint8_t, Color) const = 0;
        virtual void drawButtonFrame(bool, const Rect&) const = 0;
        virtual void drawButtonBackground(bool, const Rect&) const = 0;
        virtual void drawButtonLabel(const char*, bool, const Rect&) const = 0;
        virtual Extent getProgressBarHeight() const = 0;
        virtual float getProgressBarIndeterminateStep() const = 0;
        virtual void drawProgressBarBackground(const Rect&) const = 0;
        virtual void drawProgressBarFrame(const Rect&) const = 0;
        virtual void drawProgressBarProgress(const Rect&, float) const = 0;
        virtual void drawProgressBarIndeterminate(const Rect&, float) const = 0;
    };

    using ThemePtr = std::shared_ptr<ITheme>;

    class DefaultTheme : public ITheme
    {
    public:
        TWM_CONST(Color, ScreensaverColor, 0x0000);
        TWM_CONST(Color, DesktopWindowColor, 0xb59a);
        TWM_CONST(const GFXfont*, SmallFont, &FreeSans9pt7b);
        TWM_CONST(const GFXfont*, MediumFont, &FreeSans12pt7b);
        TWM_CONST(const GFXfont*, LargeFont, &FreeSans18pt7b);
        TWM_CONST(float, SmallScaleFactor, 1.0f);
        TWM_CONST(float, MediumScaleFactor, 2.0f);
        TWM_CONST(float, LargeScaleFactor, 3.0f);
        TWM_CONST(float, WindowXPadFactor, 0.07f);
        TWM_CONST(float, WindowYPadFactor, 0.07f);
        TWM_CONST(float, ButtonWidthFactor, 0.27f);
        TWM_CONST(float, ButtonHeightFactor, ButtonWidthFactor * 0.52f);
        TWM_CONST(float, ProgressBarHeightFactor, 0.12f);
        TWM_CONST(Extent, WindowFrameThickness, 1);
        TWM_CONST(Color, WindowFrameColor, 0x9cf3);
        TWM_CONST(Color, WindowFrameShadowColor, 0xb5b6);
        TWM_CONST(Color, WindowBgColor, 0xdedb);
        TWM_CONST(Color, WindowTextColor, 0x0000);
        TWM_CONST(Color, ButtonFrameColor, 0x6b6d);
        TWM_CONST(Color, ButtonBgColor, 0x8c71);
        TWM_CONST(Color, ButtonTextColor, 0xffff);
        TWM_CONST(Extent, ButtonLabelPadding, 10);
        TWM_CONST(Color, ButtonFrameColorPressed, 0x6b6d);
        TWM_CONST(Color, ButtonBgColorPressed, 0x738e);
        TWM_CONST(Color, ButtonTextColorPressed, 0xffff);
        TWM_CONST(u_long, ButtonTappedDuration, 200);
        TWM_CONST(Coord, ButtonCornerRadius, 4);
        TWM_CONST(Color, ProgressBarBackgroundColor, 0xef5d);
        TWM_CONST(Color, ProgressBarFrameColor, 0x9cf3);
        TWM_CONST(Color, ProgressBarProgressColor, 0x0ce0);
        TWM_CONST(float, ProgressBarIndeterminateBandWidth, 0.33f);
        TWM_CONST(float, ProgressBarIndeterminateStep, 1.0f);
        TWM_CONST(Coord, ScreenThresholdSmall, 320);
        TWM_CONST(Coord, ScreenThresholdMedium, 480);
        TWM_CONST(uint8_t, WindowTextSize, 1);
        TWM_CONST(uint8_t, ButtonTextSize, 1);
        TWM_CONST(Coord, WindowTextYOffset, 4);

        void setGfxDriver(const GfxDriverPtr& gfx)
        {
            TWM_ASSERT(gfx);
            _gfx = gfx;
        }

        void drawScreensaver() const final
        {
            _gfx->fillScreen(ScreensaverColor);
        }

        void drawDesktopBackground() const final
        {
            _gfx->fillRect(0, 0, _gfx->width(), _gfx->height(), DesktopWindowColor);
        }

        void setFont(const GFXfont* font) const final { _gfx->setFont(font); }
        void setSmallFont() const final { _gfx->setFont(getSmallFont()); }
        void setMediumFont() const final { _gfx->setFont(getMediumFont()); }
        void setLargeFont() const final { _gfx->setFont(getLargeFont()); }

        const GFXfont* autoSelectFont() const final
        {
            const GFXfont* font = nullptr;
            switch (getScreenSize()) {
                default:
                case ScreenSize::Small:
                    font = getSmallFont();
                break;
                case ScreenSize::Medium:
                    font = getMediumFont();
                break;
                case ScreenSize::Large:
                    font = getLargeFont();
                break;
            }
            setFont(font);
            return font;
        }

        const GFXfont* getSmallFont() const final { return SmallFont; }
        const GFXfont* getMediumFont() const final { return MediumFont; }
        const GFXfont* getLargeFont() const final { return LargeFont; }

        ScreenSize getScreenSize() const final
        {
            if (_gfx->width() <= ScreenThresholdSmall) {
                return ScreenSize::Small;
            } else if (_gfx->width() <= ScreenThresholdMedium) {
                return ScreenSize::Medium;
            } else {
                return ScreenSize::Large;
            }
        }

        Extent getScaledValue(Extent value) const final
        {
            switch (getScreenSize()) {
                default:
                case ScreenSize::Small:
                    return abs(value * SmallScaleFactor);
                case ScreenSize::Medium:
                    return abs(value * MediumScaleFactor);
                case ScreenSize::Large:
                    return abs(value * LargeScaleFactor);
            }
        }

        Extent getWindowXPadding() const final
        {
            return abs(_gfx->width() * WindowXPadFactor);
        }

        Extent getWindowYPadding() const final
        {
            return abs(_gfx->height() * WindowYPadFactor);
        }

        Extent getButtonWidth() const final
        {
            return abs(_gfx->width() * ButtonWidthFactor);
        }

        Extent getButtonHeight() const final
        {
            return abs(_gfx->height() * ButtonHeightFactor);
        }

        Coord getButtonCornerRadius() const final
        {
            switch (getScreenSize()) {
                default:
                case ScreenSize::Small:
                    return ButtonCornerRadius;
                case ScreenSize::Medium:
                    return ButtonCornerRadius + 1;
                case ScreenSize::Large:
                    return ButtonCornerRadius + 2;
            }
        }

        uint8_t getButtonTextSize() const final { return ButtonTextSize; }
        Color getButtonTextColor() const final { return ButtonTextColor; }
        Color getButtonTextColorPressed() const final { return ButtonTextColorPressed; }
        Color getButtonBgColor() const final { return ButtonBgColor; }
        Color getButtonBgColorPressed() const final { return ButtonBgColorPressed; }
        Color getButtonFrameColor() const final { return ButtonFrameColor; }
        Color getButtonFrameColorPressed() const final { return ButtonFrameColorPressed; }
        Extent getButtonLabelPadding() const final { return ButtonLabelPadding; }
        u_long getButtonTappedDuration() const final { return ButtonTappedDuration; }
        uint8_t getWindowTextSize() const final { return WindowTextSize; }
        Color getWindowTextColor() const final { return WindowTextColor; }

        Extent getWindowFrameThickness() const final
        {
            return getScaledValue(WindowFrameThickness);
        }

        void drawWindowFrame(const Rect& rect) const final
        {
            Rect tmp = rect;
            auto pixels = getWindowFrameThickness();
            while (pixels-- > 0) {
                _gfx->drawRect(tmp.left, tmp.top, tmp.width(), tmp.height(),
                    WindowFrameColor);
                tmp.deflate(1);
            }
            _gfx->drawLine(
                rect.left + 1,
                rect.bottom,
                rect.left + (rect.width() - 1),
                rect.bottom,
                WindowFrameShadowColor
            );
            _gfx->drawLine(
                rect.right,
                rect.top + 1,
                rect.right,
                rect.top + (rect.height() - 1),
                WindowFrameShadowColor
            );
        }

        void drawWindowBackground(const Rect& rect) const final
        {
            _gfx->fillRect(rect.left, rect.top, rect.width(), rect.height(),
                WindowBgColor);
        }

        void drawText(const char* text, uint8_t flags, const Rect& rect,
            uint8_t textSize, Color textColor) const final
        {
            bool xCenter = bitsHigh(flags, DTF_CENTER);
            bool singleLine = bitsHigh(flags, DTF_SINGLE);

            uint8_t xAdv = 0, yAdv = 0, yAdvMax = 0;
            int8_t xOff = 0, yOff = 0, yOffMin = 0;
            Extent xAccum = 0;
            Extent yAccum =
                rect.top + (singleLine ? (rect.height() / 2) : getWindowYPadding())
                    + WindowTextYOffset;
            const Extent xPadding =
                ((singleLine && !xCenter) ? 0 : getWindowXPadding());
            const Extent xExtent = rect.right - (xPadding * 2);

            const char* cursor = text;
            while (*cursor != '\0') {
                xAccum = rect.left + xPadding;
                const char* old_cursor = cursor;
                std::vector<uint8_t> charXAdvs;
                while (xAccum < xExtent && *cursor != '\0') {
                    AdafruitExt_getCharBounds(
                        *cursor,
                        nullptr,
                        nullptr,
                        &xAdv,
                        &yAdv,
                        &xOff,
                        &yOff,
                        textSize,
                        textSize,
                        autoSelectFont()
                    );
                    charXAdvs.push_back(xAdv);
                    xAccum += xAdv;
                    cursor++;
                    if (yAdv > yAdvMax) { yAdvMax = yAdv; }
                    if (yOff > yOffMin) { yOffMin = yOff; }
                }
                size_t rewound = 0;
                if (!singleLine) {
                    for (size_t rewind = 0; rewind < (cursor - old_cursor); rewind++) {
                        if (*(cursor - rewind) == ' ') {
                            rewound = rewind;
                            cursor -= rewind;
                            while (rewind > 0) {
                                xAccum -= charXAdvs.at(charXAdvs.size() - rewind);
                                rewind--;
                            }
                            break;
                        }
                    }
                }
                const Extent drawnWidth = xAccum - (rect.left + xPadding);
                xAccum = xCenter
                    ? rect.left + (rect.width() / 2) - (drawnWidth / 2)
                    : rect.left + xPadding;
                while (old_cursor < cursor) {
                    _gfx->drawChar(xAccum, yAccum, *old_cursor++, textColor,
                        textColor, textSize);
                    xAccum += charXAdvs[
                        charXAdvs.size() - 2 - ((cursor + rewound) - old_cursor - 1)
                    ];
                }
                if (!singleLine) {
                    if (rewound > 0) { cursor++; }
                    yAccum += yAdvMax + yOffMin;
                } else {
                    break;
                }
            }
        }

        void drawButtonFrame(bool pressed, const Rect& rect) const final
        {
            _gfx->drawRoundRect(
                rect.left,
                rect.top,
                rect.width(),
                rect.height(),
                getButtonCornerRadius(),
                pressed ? getButtonFrameColorPressed() : getButtonFrameColor()
            );
        }

        void drawButtonBackground(bool pressed, const Rect& rect) const final
        {
            _gfx->fillRoundRect(
                rect.left,
                rect.top,
                rect.width(),
                rect.height(),
                getButtonCornerRadius(),
                pressed ? getButtonBgColorPressed() : getButtonBgColor()
            );
        }

        void drawButtonLabel(const char* lbl, bool pressed, const Rect& rect) const final
        {
            drawText(lbl, DTF_SINGLE | DTF_CENTER, rect, getButtonTextSize(),
                pressed ? getButtonTextColorPressed() : getButtonTextColor());
        }

        Extent getProgressBarHeight() const final
        {
            return abs(_gfx->height() * ProgressBarHeightFactor);
        }

        float getProgressBarIndeterminateStep() const final
        {
            switch (getScreenSize()) {
                default:
                case ScreenSize::Small:
                    return ProgressBarIndeterminateStep;
                case ScreenSize::Medium:
                    return ProgressBarIndeterminateStep * 2;
                case ScreenSize::Large:
                    return ProgressBarIndeterminateStep * 4;
            }
        }

        void drawProgressBarBackground(const Rect& rect) const final
        {
            _gfx->fillRect(rect.left, rect.top, rect.width(), rect.height(),
                ProgressBarBackgroundColor);
        }

        void drawProgressBarFrame(const Rect& rect) const final
        {
            Rect tmp = rect;
            auto pixels = getWindowFrameThickness();
            while (pixels-- > 0) {
                _gfx->drawRect(tmp.left, tmp.top, tmp.width(), tmp.height(),
                    ProgressBarFrameColor);
                tmp.deflate(1);
            }
        }

        void drawProgressBarProgress(const Rect& rect, float percent) const final
        {
            TWM_ASSERT(percent >= 0.0f && percent <= 100.0f);
            Rect barRect = rect;
            barRect.deflate(getWindowFrameThickness() * 2);
            float progressWidth = (barRect.width() * (min(100.0f, percent) / 100.0f));
            barRect.right = barRect.left + abs(progressWidth);
            _gfx->fillRect(barRect.left, barRect.top, barRect.width(),
                barRect.height(), ProgressBarProgressColor);
        }

        void drawProgressBarIndeterminate(const Rect& rect, float counter) const final
        {
            TWM_ASSERT(counter >= 0.0f && counter <= 100.0f);
            Rect barRect = rect;
            barRect.deflate(getWindowFrameThickness() * 2);
            Extent bandWidth
                = (barRect.width() * ProgressBarIndeterminateBandWidth);
            Coord offset
                = (barRect.width() + bandWidth) * (min(100.0f, counter) / 100.0f);
            static Coord reverseOffset = bandWidth;

            Coord x = 0;
            Extent width = 0;
            if (offset < bandWidth) {
                x = barRect.left;
                if (counter <= __FLT_EPSILON__) {
                    reverseOffset = bandWidth;
                }
                width = offset;
            } else {
                Coord realOffset = reverseOffset > 0
                    ? offset - reverseOffset--
                    : offset;
                x = min(
                    static_cast<Coord>(barRect.left + realOffset),
                    barRect.right
                );
                width = min(
                    bandWidth,
                    static_cast<Extent>(barRect.right - x)
                );
            }
            _gfx->fillRect(x, barRect.top, width, barRect.height(),
                ProgressBarProgressColor);
        }

    private:
        GfxDriverPtr _gfx;
    };

    struct PackagedMessage
    {
        Message msg = MSG_NONE;
        MsgParam p1 = 0;
        MsgParam p2 = 0;
    };

    using PackagedMessageQueue = std::queue<PackagedMessage>;

    class IWindow;
    class IWindowContainer
    {
    public:
        virtual bool hasChildren() = 0;
        virtual size_t childCount() = 0;
        virtual std::shared_ptr<IWindow> getChildByID(WindowID id) = 0;
        virtual bool addChild(const std::shared_ptr<IWindow>& child) = 0;
        virtual bool removeChildByID(WindowID id) = 0;
        virtual void removeAllChildren() = 0;
        virtual void forEachChild(const std::function<bool(const std::shared_ptr<IWindow>&)>& cb) = 0;
        virtual void forEachChildReverse(const std::function<bool(const std::shared_ptr<IWindow>&)>& cb) = 0;
    };

    class IWindow : public IWindowContainer
    {
    public:
        virtual std::shared_ptr<IWindow> getParent() const = 0;
        virtual void setParent(const std::shared_ptr<IWindow>& parent) = 0;

        virtual Rect getRect() const noexcept = 0;
        virtual void setRect(const Rect& rect) noexcept = 0;

        virtual Style getStyle() const noexcept = 0;
        virtual void setStyle(Style style) noexcept = 0;

        virtual WindowID getID() const noexcept = 0;

        virtual State getState() const noexcept = 0;
        virtual void setState(State state) noexcept = 0;

        virtual std::string getText() const = 0;
        virtual void setText(const std::string& text) = 0;

        virtual bool routeMessage(Message msg, MsgParam p1 = 0, MsgParam p2 = 0) = 0;
        virtual bool queueMessage(Message msg, MsgParam p1 = 0, MsgParam p2 = 0) = 0;
        virtual bool processQueue() = 0;

        virtual bool redraw() = 0;
        virtual bool hide() noexcept = 0;
        virtual bool show() noexcept = 0;
        virtual bool isVisible() const noexcept = 0;
        virtual bool processInput(InputParams* params) = 0;
        virtual bool destroy() = 0;
        virtual bool isAlive() const noexcept = 0;

    protected:
        virtual bool onCreate(MsgParam p1, MsgParam p2) = 0;
        virtual bool onDestroy(MsgParam p1, MsgParam p2) = 0;
        virtual bool onDraw(MsgParam p1, MsgParam p2) = 0;
        virtual bool onInput(MsgParam p1, MsgParam p2) = 0;
        virtual bool onEvent(MsgParam p1, MsgParam p2) = 0;
        virtual bool onResize(MsgParam p1, MsgParam p2) = 0;

        virtual bool onTapped(Coord x, Coord y) = 0;
    };

    using WindowPtr          = std::shared_ptr<IWindow>;
    using WindowContainerPtr = std::shared_ptr<IWindowContainer>;

    class WindowContainer : public IWindowContainer
    {
    public:
        using WindowStack = std::vector<WindowPtr>;

        WindowContainer() = default;
        virtual ~WindowContainer() = default;

        bool hasChildren() override
        {
# if !defined(TWM_SINGLETHREAD)
            ScopeLock lock(_childMtx);
# endif
            return !_children.empty();
        }

        size_t childCount() override
        {
# if !defined(TWM_SINGLETHREAD)
            ScopeLock lock(_childMtx);
# endif
            return _children.size();
        }

        WindowPtr getChildByID(WindowID id) override
        {
# if !defined(TWM_SINGLETHREAD)
            ScopeLock lock(_childMtx);
# endif
            for (auto win : _children) {
                if (id == win->getID()) {
                    return win;
                }
            }
            return nullptr;
        }

        bool addChild(const WindowPtr& child) override
        {
# if !defined(TWM_SINGLETHREAD)
            ScopeLock lock(_childMtx);
# endif
            if (getChildByID(child->getID()) != nullptr) {
                return false;
            }
            _children.push_back(child);
            return true;
        }

        bool removeChildByID(WindowID id) override
        {
# if !defined(TWM_SINGLETHREAD)
            ScopeLock lock(_childMtx);
# endif
            for (auto it = _children.begin(); it != _children.end(); it++) {
                if (id == (*it)->getID()) {
                    _children.erase(it);
                    return true;
                }
            }
            return false;
        }

        void removeAllChildren() override
        {
# if !defined(TWM_SINGLETHREAD)
            ScopeLock lock(_childMtx);
# endif
            _children.clear();
        }

        void forEachChild(const std::function<bool(const WindowPtr&)>& cb) override
        {
            if (!cb) {
                return;
            }
# if !defined(TWM_SINGLETHREAD)
            ScopeLock lock(_childMtx);
# endif
            for (auto it = _children.begin(); it != _children.end(); it++) {
                if (!cb((*it))) {
                    break;
                }
            }
        }

        void forEachChildReverse(const std::function<bool(const WindowPtr&)>& cb) override
        {
            if (!cb) {
                return;
            }
# if !defined(TWM_SINGLETHREAD)
            ScopeLock lock(_childMtx);
# endif
            for (auto it = _children.rbegin(); it != _children.rend(); it++) {
                if (!cb((*it))) {
                    break;
                }
            }
        }

    protected:
        WindowStack _children;
# if !defined(TWM_SINGLETHREAD)
        Mutex _childMtx;
# endif
    };

    class TWM : public std::enable_shared_from_this<TWM>
    {
    public:
        TWM() = delete;

        explicit TWM(const GfxDriverPtr& gfx, const ThemePtr& theme)
            : _gfx(gfx), _theme(theme)
        {
            TWM_ASSERT(_gfx);
            TWM_ASSERT(_theme);
            if (_theme && _gfx) {
                _theme->setGfxDriver(_gfx);
                _theme->autoSelectFont();
            }
            _registry = std::make_shared<WindowContainer>();
            TWM_ASSERT(_registry);
        }

        virtual ~TWM()
        {
            tearDown();
        }

        virtual void tearDown()
        {
            _registry->forEachChild([](const WindowPtr& child)
            {
                child->destroy();
                return true;
            });
            _registry->removeAllChildren();
        }

        GfxDriverPtr getGfx() const { return _gfx; }
        ThemePtr getTheme() const { return _theme; }

        Extent getScreenWidth() const { return _gfx->width(); }
        Extent getScreenHeight() const { return _gfx->height(); }

        virtual void update()
        {
            _theme->drawDesktopBackground();
            _registry->forEachChild([](const WindowPtr& win)
            {
                win->processQueue();
                // TODO: is this window completely covered by another, or off the screen?
                win->redraw();
                return true;
            });
        }

        template<class TWindow>
        inline std::shared_ptr<TWindow> createWindow(
            const WindowPtr& parent,
            WindowID id,
            Style style,
            Coord x,
            Coord y,
            Extent width,
            Extent height,
            const std::string& text = std::string(),
            const std::function<bool(const std::shared_ptr<TWindow>&)>& preCreateHook = nullptr
        )
        {
            Rect rect(x, y, x + width, y + height);
            std::shared_ptr<TWindow> win(
                std::make_shared<TWindow>(shared_from_this(), parent, id, style, rect, text)
            );
            if (!win) {
                TWM_LOG(TWM_ERROR, "oom");
                return nullptr;
            }
            if (bitsHigh(style, STY_CHILD) && !parent) {
                TWM_LOG(TWM_ERROR, "STY_CHILD w/ null parent");
                return nullptr;
            }
            if (preCreateHook && !preCreateHook(win)) {
                TWM_LOG(TWM_ERROR, "pre-create hook false");
                return nullptr;
            }
            if (!win->routeMessage(MSG_CREATE)) {
                TWM_LOG(TWM_ERROR, "MSG_CREATE false");
                return nullptr;
            }
            bool dupe = false;
            if (parent) {
                dupe = !parent->addChild(win);
            } else {
                dupe = !_registry->addChild(win);
            }
            if (dupe) {
                TWM_LOG(TWM_ERROR, "dupe %hhu", id);
                return nullptr;
            }
            win->setState(win->getState() | STA_ALIVE);
            if (bitsHigh(win->getStyle(), STY_AUTOSIZE)) {
                win->routeMessage(MSG_RESIZE);
            }
            if (win->isVisible() && (!parent || parent->isVisible())) {
                win->redraw();
            }
            return win;
        }

        template<class TPrompt>
        inline std::shared_ptr<TPrompt> createPrompt(
            const WindowPtr& parent,
            WindowID id,
            Style style,
            const std::string& text,
            const std::vector<typename TPrompt::ButtonInfo>& buttonInfo,
            const typename TPrompt::ResultCallback& callback
        )
        {
            TWM_ASSERT(bitsHigh(style, STY_PROMPT));
            auto prompt = createWindow<TPrompt>(
                parent,
                id,
                style,
                _theme->getWindowXPadding(),
                _theme->getWindowYPadding(),
                _gfx->width() - (_theme->getWindowXPadding() * 2),
                _gfx->height() - (_theme->getWindowYPadding() * 2),
                text,
                [&](const std::shared_ptr<TPrompt>& win)
                {
                    for (const auto& btn : buttonInfo) {
                        if (!win->addButton(btn)) {
                            return false;
                        }
                    }
                    win->setResultCallback(callback);
                    return true;
                }
            );
            return prompt;
        }

        template<class TBar>
        inline std::shared_ptr<TBar> createProgressBar(
            const WindowPtr& parent,
            WindowID id,
            Style style,
            Coord x,
            Coord y,
            Extent width,
            Extent height,
            Style pbarStyle
        )
        {
            TWM_ASSERT(bitsHigh(style, STY_PROGBAR));
            auto pbar = createWindow<TBar>(parent, id, style, x, y, width, height);
            if (pbar) {
                pbar->setProgressBarStyle(pbarStyle);
            }
            return pbar;
        }

        void hitTest(Coord x, Coord y)
        {
            //TWM_LOG(TWM_DEBUG, "hit test @ %hd, %hd...", x, y);
            _registry->forEachChildReverse([&](const WindowPtr& child)
            {
                InputParams params;
                params.type = INPUT_TAP;
                params.x    = x;
                params.y    = y;
                if (child->processInput(&params)) {
                    //TWM_LOG(TWM_DEBUG, "%hhu claimed hit test @ %hd, %hd",
                    //    params.handledBy, x, y);
                    return false;
                }
                return true;
            });
        }

    protected:
        WindowContainerPtr _registry;
        GfxDriverPtr _gfx;
        ThemePtr _theme;
    };

    using TWMPtr = std::shared_ptr<TWM>;

    class Window : public IWindow, public std::enable_shared_from_this<IWindow>
    {
    public:
        Window() = default;

        Window(
            const TWMPtr& wm,
            const WindowPtr& parent,
            WindowID id,
            Style style,
            const Rect& rect,
            const std::string& text
        ) : _wm(wm), _parent(parent), _rect(rect), _style(style), _id(id), _text(text)
        {
        }

        virtual ~Window() = default;

        bool hasChildren() override {
            return _children.hasChildren();
        }

        size_t childCount() override
        {
            return _children.childCount();
        }

        WindowPtr getChildByID(WindowID id) override
        {
            return _children.getChildByID(id);
        }

        bool addChild(const WindowPtr& child) override
        {
            return _children.addChild(child);
        }

        bool removeChildByID(WindowID id) override
        {
            return _children.removeChildByID(id);
        }

        void removeAllChildren() override
        {
            return _children.removeAllChildren();
        }

        void forEachChild(const std::function<bool(const WindowPtr&)>& cb) override
        {
            return _children.forEachChild(cb);
        }

        void forEachChildReverse(const std::function<bool(const WindowPtr&)>& cb) override
        {
            return _children.forEachChildReverse(cb);
        }

        WindowPtr getParent() const override { return _parent; }
        void setParent(const WindowPtr& parent) override { _parent = parent; }

        Rect getRect() const noexcept override { return _rect; }
        void setRect(const Rect& rect) noexcept override { _rect = rect; }

        Style getStyle() const noexcept override { return _style; }
        void setStyle(Style style) noexcept override { _style = style; }

        WindowID getID() const noexcept override { return _id; }

        State getState() const noexcept override { return _state; }
        void setState(State state) noexcept override { _state = state; }

        std::string getText() const override { return _text; }
        void setText(const std::string& text) override { _text = text; }

        bool routeMessage(Message msg, MsgParam p1 = 0, MsgParam p2 = 0) override
        {
            switch (msg) {
                case MSG_CREATE:  return onCreate(p1, p2);
                case MSG_DESTROY: return onDestroy(p1, p2);
                case MSG_DRAW: {
                    auto parent = getParent();
                    if (!isVisible() || !isAlive() ||
                        (parent && (!parent->isVisible() || !parent->isAlive()))) {
                        return false;
                    }
                    return onDraw(p1, p2);
                }
                case MSG_INPUT:   return onInput(p1, p2);
                case MSG_EVENT:   return onEvent(p1, p2);
                case MSG_RESIZE:  return onResize(p1, p2);
                default:
                    TWM_ASSERT(false);
                return false;
            }
        }

        bool queueMessage(Message msg, MsgParam p1 = 0, MsgParam p2 = 0) override
        {
# if !defined(TWM_SINGLETHREAD)
            ScopeLock lock(_queueMtx);
# endif
            PackagedMessage pm;
            pm.msg = msg;
            pm.p1 = p1;
            pm.p2 = p2;
            _queue.push(pm);
            if (pm.msg == MSG_INPUT) {
                return getMsgParamLoWord(pm.p1) == INPUT_TAP;
            }
            return false;
        }

        bool processQueue() override
        {
# if !defined(TWM_SINGLETHREAD)
            ScopeLock lock(_queueMtx);
# endif
            if (!_queue.empty()) {
                auto pm = _queue.front();
                _queue.pop();
                routeMessage(pm.msg, pm.p1, pm.p2);
            }
            forEachChild([&](const WindowPtr& child)
            {
                child->processQueue();
                return true;
            });
            return !_queue.empty();
        }

        bool redraw() override
        {
            if (!isVisible() || !isAlive()) {
                return false;
            }
            bool redrawn = routeMessage(MSG_DRAW);
            forEachChild([&](const WindowPtr& child)
            {
                redrawn &= child->redraw();
                return true;
            });
            return redrawn;
        }

        bool hide() noexcept override
        {
            if (!isVisible()) {
                return false;
            }
            setStyle(getStyle() & ~STY_VISIBLE);
            return true;
        }

        bool show() noexcept override
        {
            if (isVisible()) {
                return false;
            }
            setStyle(getStyle() | STY_VISIBLE);
            return redraw();
        }

        bool isVisible() const noexcept override
        {
            return bitsHigh(getStyle(), STY_VISIBLE);
        }

        bool processInput(InputParams* params) override
        {
            if (!isVisible() || !isAlive()) {
                return false;
            }
            Rect rect = getRect();
            if (!rect.isPointWithin(params->x, params->y)) {
                return false;
            }
            bool handled = false;
            forEachChildReverse([&](const WindowPtr& child)
            {
                handled = child->processInput(params);
                if (handled) {
                    return false;
                }
                return true;
            });
            if (!handled) {
                handled = queueMessage(
                    MSG_INPUT,
                    makeMsgParam(0, params->type),
                    makeMsgParam(params->x, params->y)
                );
                if (handled) {
                    params->handledBy = getID();
                }
            }
            return handled;
        }

        bool destroy() override
        {
            bool destroyed = routeMessage(MSG_DESTROY);
            forEachChild([&](const WindowPtr& child)
            {
                destroyed &= child->destroy();
                return true;
            });
            removeAllChildren();
            return destroyed;
        }

        bool isAlive() const noexcept override
        {
            return bitsHigh(getState(), STA_ALIVE);
        }

    protected:
        /** MSG_CREATE: param1 = nullptr, param2 = nullptr. */
        bool onCreate(MsgParam p1, MsgParam p2) override { return true; }

        /** MSG_DESTROY: param1 = nullptr, param2 = nullptr. */
        bool onDestroy(MsgParam p1, MsgParam p2) override
        {
            setState(getState() & ~STA_ALIVE);
            return true;
        }

        /** MSG_DRAW: param1 = nullptr, param2 = nullptr. */
        bool onDraw(MsgParam p1, MsgParam p2) override
        {
            auto theme = _getTheme();
            if (theme) {
                Rect rect = getRect();
                TWM_ASSERT(rect.width() > 0 && rect.height() > 0);
                theme->drawWindowBackground(rect);
                theme->drawWindowFrame(rect);
                return true;
            }
            return false;
        }

        /** MSG_INPUT: param1 = (loword: type), param2 = (hiword: x, loword: y).
         * Returns true if the input event was consumed by this window, false otherwise. */
        bool onInput(MsgParam p1, MsgParam p2) override
        {
            InputParams params;
            params.type = static_cast<InputType>(getMsgParamLoWord(p1));
            params.x    = getMsgParamHiWord(p2);
            params.y    = getMsgParamLoWord(p2);

            bool handled = false;
            switch (params.type) {
                case INPUT_TAP:
                    handled = onTapped(params.x, params.y);
                break;
                case INPUT_NONE:
                default:
                    TWM_ASSERT(false);
                break;
            }
            return handled;
        }

        /** MSG_EVENT: param1 = EventType, param2 = child WindowID. */
        bool onEvent(MsgParam p1, MsgParam p2) override { return true; }

        bool onResize(MsgParam p1, MsgParam p2) override
        {
            TWM_ASSERT(bitsHigh(getStyle(), STY_AUTOSIZE));
            return false;
        }

        bool onTapped(Coord x, Coord y) override { return false; }

        TWMPtr _getWM() const
        {
            TWM_ASSERT(_wm);
            return _wm;
        }

        GfxDriverPtr _getGfx() const
        {
            auto wm = _getWM();
            if (wm) {
                auto gfx = wm->getGfx();
                TWM_ASSERT(gfx);
                return gfx;
            }
            return nullptr;
        }

        ThemePtr _getTheme() const
        {
            auto wm = _getWM();
            if (wm) {
                auto theme = wm->getTheme();
                TWM_ASSERT(theme);
                return theme;
            }
            return nullptr;
        }

    protected:
        WindowContainer _children;
        PackagedMessageQueue _queue;
# if !defined(TWM_SINGLETHREAD)
        Mutex _queueMtx;
# endif
        TWMPtr _wm;
        WindowPtr _parent;
        Rect _rect;
        Style _style = 0;
        WindowID _id = 0;
        State _state = 0;
        std::string _text;
    };

    class Button : public Window
    {
    public:
        using Window::Window;
        Button() = default;
        virtual ~Button() = default;

        bool onTapped(Coord x, Coord y) override
        {
            _lastTapped = millis();
            routeMessage(MSG_DRAW);
            auto parent = getParent();
            TWM_ASSERT(parent);
            if (parent) {
                parent->queueMessage(
                    MSG_EVENT,
                    EVT_CHILD_TAPPED,
                    getID()
                );
            }
            return true;
        }

        bool onDraw(MsgParam p1, MsgParam p2) override
        {
            auto theme = _getTheme();
            if (theme) {
                bool pressed = (millis() - _lastTapped < theme->getButtonTappedDuration());
                Rect rect = getRect();
                theme->drawButtonBackground(pressed, rect);
                theme->drawButtonFrame(pressed, rect);
                theme->drawButtonLabel(getText().c_str(), pressed, rect);
                return true;
            }
            return false;
        }

        bool onResize(MsgParam p1, MsgParam p2) override
        {
            auto gfx = _getGfx();
            auto theme = _getTheme();
            if (gfx && theme) {
                gfx->setTextSize(theme->getButtonTextSize());
                Coord x, y;
                Extent width, height;
                gfx->getTextBounds(getText().c_str(), 0, 0, &x, &y, &width, &height);
                Rect rect = getRect();
                rect.right = rect.left + max(width, theme->getButtonWidth()) + (theme->getButtonLabelPadding() * 2);
                rect.bottom = rect.top + theme->getButtonHeight();
                setRect(rect);
                // TODO: if not autosize, clip label, perhaps with ellipsis.
                return true;
            }
            return false;
        }

    protected:
        u_long _lastTapped = 0UL;
    };

    class Label : public Window
    {
    public:
        TWM_CONST(uint8_t, DrawTextFlags, DTF_SINGLE);

        using Window::Window;
        Label() = default;
        virtual ~Label() = default;

        bool onDraw(MsgParam p1, MsgParam p2) override
        {
            auto theme = _getTheme();
            if (theme) {
                Rect rect = getRect();
                theme->drawWindowBackground(rect);
                theme->drawText(getText().c_str(), DrawTextFlags, rect,
                    theme->getWindowTextSize(), theme->getWindowTextColor());
                return true;
            }
            return false;
        }
    };

    class MultilineLabel : public Window
    {
    public:
        TWM_CONST(uint8_t, DrawTextFlags, DTF_CENTER);

        using Window::Window;
        MultilineLabel() = default;
        virtual ~MultilineLabel() = default;

        bool onDraw(MsgParam p1, MsgParam p2) override
        {
            auto theme = _getTheme();
            if (theme) {
                Rect rect = getRect();
                theme->drawWindowBackground(rect);
                theme->drawText(getText().c_str(), DrawTextFlags, rect,
                    theme->getWindowTextSize(), theme->getWindowTextColor());
                return true;
            }
            return false;
        }
    };

    class Prompt : public Window
    {
    public:
        using ButtonInfo     = std::pair<WindowID, std::string>;
        using ResultCallback = std::function<void(WindowID)>;

        using Window::Window;
        Prompt() = default;
        virtual ~Prompt() = default;

        void setResultCallback(const ResultCallback& callback)
        {
            _callback = callback;
        }

        bool addButton(const ButtonInfo& bi)
        {
            auto wm = _getWM();
            if (wm) {
                auto theme = _getTheme();
                if (theme) {
                    auto btn = wm->createWindow<Button>(
                        shared_from_this(),
                        bi.first,
                        STY_CHILD | STY_VISIBLE | STY_AUTOSIZE | STY_BUTTON,
                        0,
                        0,
                        0,
                        0,
                        bi.second
                    );
                    return btn != nullptr;
                }
            }
            return false;
        }

        void setText(const std::string& text) override
        {
            if (_label) {
                _label->setText(text);
            }
        }

        bool onCreate(MsgParam param1, MsgParam param2) override
        {
            auto wm = _getWM();
            if (wm) {
                auto theme = _getTheme();
                if (theme) {
                    Rect rect = getRect();
                    _label = wm->createWindow<MultilineLabel>(
                        shared_from_this(),
                        WID_PROMPTLBL,
                        STY_CHILD | STY_VISIBLE | STY_LABEL,
                        rect.left + theme->getWindowXPadding(),
                        rect.top + theme->getWindowYPadding(),
                        rect.right - rect.left - (theme->getWindowXPadding() * 2),
                        rect.bottom - rect.top - ((theme->getWindowYPadding() * 3) + theme->getButtonHeight()),
                        getText()
                    );
                    if (!_label) {
                        return false;
                    }
                    Rect rectLbl = _label->getRect();
                    bool first = true;
                    uint8_t numButtons = 0;
                    forEachChild([&](const WindowPtr& child)
                    {
                        if (bitsHigh(child->getStyle(), STY_BUTTON)) {
                            numButtons++;
                        }
                        return true;
                    });
                    forEachChild([&](const WindowPtr& child)
                    {
                        if (!bitsHigh(child->getStyle(), STY_BUTTON)) {
                            return true;
                        }
                        Rect rectBtn = child->getRect();
                        rectBtn.top = rectLbl.bottom + theme->getWindowYPadding();
                        rectBtn.bottom = rectBtn.top + theme->getButtonHeight();
                        auto width = rectBtn.width();
                        if (first) {
                            first = false;
                            switch (numButtons) {
                                case 1: {
                                    rectBtn.left
                                        = rect.left + (rect.width() / 2) - (width / 2);
                                }
                                break;
                                case 2: {
                                    rectBtn.left = rect.left + theme->getWindowXPadding();
                                }
                                break;
                                default:
                                    TWM_ASSERT(false);
                                return false;
                            }
                            rectBtn.right = rectBtn.left + width;
                        } else {
                            rectBtn.right = rect.right - theme->getWindowXPadding();
                            rectBtn.left = rectBtn.right - width;
                        }
                        child->setRect(rectBtn);
                        return true;
                    });

                    return true;
                }
            }
            return false;
        }

        bool onEvent(MsgParam param1, MsgParam param2) override
        {
            switch (static_cast<EventType>(param1)) {
                case EVT_CHILD_TAPPED: {
                    hide();
                    if (_callback) {
                        _callback(static_cast<WindowID>(param2));
                    }
                }
                break;
                default:
                    TWM_ASSERT(!"unknown event type");
                break;
            }
            return true;
        }

    protected:
        WindowPtr _label;
        ResultCallback _callback;
    };

    class ProgressBar : public Window
    {
    public:
        using Window::Window;
        ProgressBar() = default;
        virtual ~ProgressBar() = default;

        Style getProgressBarStyle() const noexcept { return _pbarStyle; }
        void setProgressBarStyle(Style pbarStyle) noexcept { _pbarStyle = pbarStyle; }

        float getProgressValue() const noexcept { return _value; }
        void setProgressValue(float value) noexcept { _value = value; }

    protected:
        bool onDraw(MsgParam p1, MsgParam p2) override
        {
            auto theme = _getTheme();
            if (theme) {
                Rect rect = getRect();
                theme->drawProgressBarBackground(rect);
                theme->drawProgressBarFrame(rect);
                if (bitsHigh(getProgressBarStyle(), PBR_NORMAL)) {
                    theme->drawProgressBarProgress(rect, getProgressValue());
                    return true;
                }
                if (bitsHigh(getProgressBarStyle(), PBR_INDETERMINATE)) {
                    theme->drawProgressBarIndeterminate(rect, getProgressValue());
                    return true;
                }
            }
            return false;
        }

        Style _pbarStyle = 0;
        float _value = 0;
    };
} // namespace twm

#endif // !_TWM_H_INCLUDED
