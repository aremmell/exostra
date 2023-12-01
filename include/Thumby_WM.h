/*
 * Thumby_WM.h
 *
 * Thumby Window Manager
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
#ifndef _THUMBY_WM_H_INCLUDED
# define _THUMBY_WM_H_INCLUDED

# include <Arduino.h>
# include <cstdint>
# include <functional>
# include <type_traits>
# include <string>
# include <memory>
# include <queue>
# include <mutex>

// TODO: remove me
# define TWM_COLOR_565

// Comment out to enable mutexes in multi-threaded environments.
#define TWM_SINGLETHREAD

// Uncomment to enable serial logging.
# define TWM_ENABLE_LOGGING

# if defined(TWM_ENABLE_LOGGING)
    enum
    {
        TWM_ERROR = 1,
        TWM_WARN  = 2,
        TWM_DEBUG = 3
    };
#  define TWM_LOG(lvl, fmt, ...) \
    do { \
        char prefix = '\0'; \
        switch(lvl) { \
            case TWM_ERROR: prefix = 'E'; break; \
            case TWM_WARN: prefix  = 'W'; break; \
            case TWM_DEBUG: prefix = 'D'; break; \
        } \
        Serial.printf("[%c] (%s:%d): ", prefix, basename(__FILE__), __LINE__); \
        Serial.printf(fmt "\n" __VA_OPT__(,) __VA_ARGS__); \
    } while(false)
#  define TWM_ASSERT(expr) \
    do { \
        if (!(expr)) { \
            TWM_LOG(TWM_ERROR, "!!! ASSERT: '" #expr "'"); \
        } \
    } while (false)
# else
#  define TWM_LOG(lvl, fmt, ...)
#  define TWM_ASSERT(expr)
# endif

# define TWM_CONST(type, name, value) \
    static constexpr PROGMEM type name = value

# if defined(TWM_GFX_ADAFRUIT) && __has_include(<Adafruit_GFX.h>)
#  include <Adafruit_GFX.h>
#  if __has_include(<Adafruit_SPITFT.h>)
#   define TWM_DISP_TYPE Adafruit_SPITFT
#  else
#   define TWM_DISP_TYPE Adafruit_GFX
#  endif
#  define TWM_CTX1_TYPE  GFXcanvas1
#  define TWM_CTX8_TYPE  GFXcanvas8
#  define TWM_CTX16_TYPE GFXcanvas16
#   if defined(__AVR__)
#    include <avr/pgmspace.h>
#   elif defined(ESP8266) || defined(ESP32)
#    include <pgmspace.h>
#   endif
#   if !defined(pgm_read_byte)
#    define pgm_read_byte(addr) (*(reinterpret_cast<const uint8_t*>(addr)))
#   endif
#   if !defined(pgm_read_word)
#    define pgm_read_word(addr) (*(reinterpret_cast<const uint16_t*>(addr)))
#   endif
#   if !defined(pgm_read_dword)
#    define pgm_read_dword(addr) (*(reinterpret_cast<const uint32_t*>(addr)))
#   endif
#   if !defined(pgm_read_pointer)
#    if !defined(__INT_MAX__) || (__INT_MAX__ > 0xffff)
#     define pgm_read_pointer(addr) (static_cast<void*>(pgm_read_dword(addr)))
#    else
#     define pgm_read_pointer(addr) (static_cast<void*>(pgm_read_word(addr)))
#    endif
#   endif
# elif defined(TWM_GFX_ARDUINO) && __has_include(<Arduino_GFX_Library.h>)
#  include <Arduino_GFX_Library.h>
#  if defined(LITTLE_FOOT_PRINT)
#   error "required Arduino GFX canvas classes unavailable for this board"
#  endif
#  if defined(ATTINY_CORE)
#   error "required GFXfont implementation unavailable for this board"
#  endif
#  define TWM_DISP_TYPE  Arduino_GFX
#  define TWM_CTX1_TYPE  Arduino_Canvas_Mono
#  define TWM_CTX8_TYPE  Arduino_Canvas_Indexed
#  define TWM_CTX16_TYPE Arduino_Canvas
# else
#  error "define TWM_GFX_ADAFRUIT or TWM_GFX_ARDUINO, and install the relevant \
library in order to select a low-level graphics driver"
# endif

namespace thumby
{
    /** Window identifier. */
    using WindowID = uint8_t;

    /** Reserved window identifiers. */
    TWM_CONST(WindowID, WID_INVALID,  -1);
    TWM_CONST(WindowID, WID_PROMPTLBL, 0);

    /** Window style bitmask. */
    using Style = uint16_t;

    /** State bitmask. */
    using State = uint16_t;

    /** Window message parameter type. */
    using MsgParam = uint32_t;

    /** Window message parameter component type. */
    using MsgParamWord = uint16_t;

    /** Physical display driver. */
    using IGfxDisplay = TWM_DISP_TYPE;

# if defined(TWM_COLOR_MONOCHROME)
    using Color       = uint8_t;       /**< Color (monochrome). */
    using IGfxContext = TWM_CTX1_TYPE; /**< Graphics context (monochrome). */
# elif defined(TWM_COLOR_256)
    using Color       = uint8_t;       /**< Color (8-bit). */
    using IGfxContext = TWM_CTX8_TYPE; /**< Graphics context (8-bit). */
# elif defined(TWM_COLOR_565)
    using Color       = uint16_t;       /**< Color type (16-bit 565 RGB). */
    using IGfxContext = TWM_CTX16_TYPE; /**< Graphics context (16-bit 565 RGB). */
# else
#  error "define TWM_COLOR_565, TWM_COLOR_256, or TWM_COLOR_MONOCHROME in order \
to select a color mode"
# endif

    /** Pointer to graphics context (e.g. canvas/buffer). */
    using GfxContextPtr = std::shared_ptr<IGfxContext>;

    /** Pointer to physical display driver. */
    using GfxDisplayPtr = std::shared_ptr<IGfxDisplay>;

    /** Font type. */
    using Font = GFXfont;

    /** Coordinate in 3D space (e.g. X, Y, or Z). */
    using Coord = int16_t;

    /** Extent (e.g. width, height). */
    using Extent = uint16_t;

    /** Point in 2D space. */
    struct Point
    {
        template<typename T1, typename T2>
        Point(T1 xAxis, T2 yAxis)
        {
            x = static_cast<Coord>(xAxis);
            y = static_cast<Coord>(yAxis);
        }

        Coord x = 0; /**< X-axis value. */
        Coord y = 0; /**< Y-axis value. */
    };

    /** Two points in 2D space (left/top, right/bottom). */
    struct Rect
    {
        Rect() = default;

        Rect(Coord l, Coord t, Extent r, Extent b)
            : left(l), top(t), right(r), bottom(b)
        {
        }

        Coord left   = 0; /**< X-axis value of the left edge. */
        Coord top    = 0; /**< Y-axis value of the top edge. */
        Coord right  = 0; /**< X-axis value of the right edge. */
        Coord bottom = 0; /**< Y-axis value of the bottom edge. */

        Extent width() const noexcept
        {
            TWM_ASSERT(right >= left);
            return static_cast<Extent>(right - left);
        }

        Extent height() const noexcept
        {
            TWM_ASSERT(bottom >= top);
            return static_cast<Extent>(bottom - top);
        }

        Point getTopLeft() const noexcept { return { left, top }; }
        Point getBottomRight() const noexcept { return { right, bottom }; }

        void inflate(Extent px) noexcept
        {
            left   -= px;
            top    -= px;
            right  += px;
            bottom += px;
        }

        void deflate(Extent px) noexcept
        {
            left   += px;
            top    += px;
            right  -= px;
            bottom -= px;
        }

        bool overlaps(const Rect& other) const noexcept
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

        bool isPointWithin(Coord x, Coord y) const noexcept
        {
            if (x >= left && x <= left + width() &&
                y >= top  && y <= top + height()) {
                return true;
            }
            return false;
        }
    };

    inline GFXglyph* getGlyphAtOffset(const GFXfont* font, uint8_t off)
    {
#   ifdef __AVR__
        return &((static_cast<GFXglyph*>(pgm_read_pointer(&font->glyph)))[off]);
#   else
        return font->glyph + off;
#   endif
    }

    static void getCharBounds(uint8_t ch, uint8_t* cx, uint8_t* cy, uint8_t* xAdv,
        uint8_t* yAdv, int8_t* xOff, int8_t* yOff, uint8_t textSize = 1,
        const GFXfont* font = nullptr)
    {
        uint8_t first = font ? pgm_read_byte(&font->first) : 0;
        bool okCh = font ? ch >= first && ch <= pgm_read_byte(&font->last) : false;
        auto glyph = okCh ? getGlyphAtOffset(font, ch - first) : nullptr;
        if (cx) { *cx = textSize * (font ? (okCh ? pgm_read_byte(&glyph->width) : 0) : 6); }
        if (cy) { *cy = textSize * (font ? (okCh ? pgm_read_byte(&glyph->height) : 0) : 8); }
        if (xAdv) { *xAdv = textSize * (okCh ? pgm_read_byte(&glyph->xAdvance) : 6); }
        if (yAdv) { *yAdv = okCh ? pgm_read_byte(&font->yAdvance) : textSize * 8; }
        if (xOff) { *xOff = okCh ? pgm_read_byte(&glyph->xOffset) : 0; }
        if (yOff) { *yOff = okCh ? pgm_read_byte(&glyph->yOffset) : 0; }
    }

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

    enum
    {
        STY_VISIBLE   = 1 << 0,
        STY_CHILD     = 1 << 1,
        STY_AUTOSIZE  = 1 << 2,
        STY_BUTTON    = 1 << 3,
        STY_LABEL     = 1 << 4,
        STY_PROMPT    = 1 << 5,
        STY_PROGBAR   = 1 << 6,
        STY_CHECKBOX  = 1 << 7
    };

    enum
    {
        PBR_NORMAL        = 1 << 0,
        PBR_INDETERMINATE = 1 << 1
    };

    enum
    {
        STA_ALIVE   = 1 << 0, /**< Active (not yet destroyed). */
        STA_CHECKED = 1 << 1  /**< Checked/highlighted item. */
    };

    enum
    {
        DT_CENTER   = 1 << 0, /**< Horizontal align center. */
        DT_SINGLE   = 1 << 1, /**< Single line of text. */
        DT_CLIP     = 1 << 2, /**< Text outside the rect will not be drawn. */
        DT_ELLIPSIS = 1 << 3  /**< Replace clipped text with '...' */
    };

    typedef enum
    {
        EVT_CHILD_TAPPED = 1
    } EventType;

    enum
    {
        INPUT_TAP = 1
    };

    struct InputParams
    {
        WindowID handledBy = WID_INVALID;
        int type;
        Coord x = 0;
        Coord y = 0;
    };

    static MsgParam makeMsgParam(const MsgParamWord& hiWord, const MsgParamWord& loWord)
    {
        return (static_cast<MsgParam>(hiWord) << 16) | (loWord & 0xffffU);
    }

    static MsgParamWord getMsgParamHiWord(const MsgParam& msgParam)
    {
        return ((msgParam >> 16) & 0xffffU);
    }

    static MsgParamWord getMsgParamLoWord(const MsgParam& msgParam)
    {
        return (msgParam & 0xffffU);
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
        virtual void setGfxContext(const GfxContextPtr&) = 0;
        virtual void drawScreensaver() const = 0;
        virtual Color getScreensaverColor() const = 0;
        virtual void drawDesktopBackground() const = 0;
        virtual Color getDesktopWindowColor() const = 0;
        virtual void setDefaultFont(const Font*) = 0;
        virtual const Font* getDefaultFont() const = 0;
        virtual void setTextSizeMultiplier(uint8_t) const = 0;
        virtual uint8_t getDefaultTextSizeMultiplier() const = 0;
        virtual ScreenSize getScreenSize() const = 0;
        virtual Extent getScaledValue(Extent) const = 0;
        virtual Extent getXPadding() const = 0;
        virtual Extent getYPadding() const = 0;
        virtual Coord getTextYOffset() const = 0;

        virtual Extent getWindowFrameThickness() const = 0;
        virtual Color getWindowTextColor() const = 0;
        virtual Color getWindowBgColor() const = 0;
        virtual Color getWindowFrameColor() const = 0;
        virtual Color getWindowFrameShadowColor() const = 0;

        virtual Extent getDefaultButtonWidth() const = 0;
        virtual Extent getDefaultButtonHeight() const = 0;
        virtual Color getButtonTextColor() const = 0;
        virtual Color getButtonTextColorPressed() const = 0;
        virtual Color getButtonBgColor() const = 0;
        virtual Color getButtonBgColorPressed() const = 0;
        virtual Color getButtonFrameColor() const = 0;
        virtual Color getButtonFrameColorPressed() const = 0;
        virtual Extent getButtonLabelPadding() const = 0;
        virtual u_long getButtonTappedDuration() const = 0;
        virtual Coord getButtonCornerRadius() const = 0;

        virtual void drawWindowFrame(const Rect&, bool) const = 0;
        virtual void drawWindowBackground(const Rect&) const = 0;
        virtual void drawText(const char*, uint8_t, const Rect&,
            uint8_t, Color, const Font*) const = 0;

        virtual void drawButtonFrame(bool, const Rect&) const = 0;
        virtual void drawButtonBackground(bool, const Rect&) const = 0;
        virtual void drawButtonLabel(const char*, bool, const Rect&) const = 0;

        virtual Extent getDefaultProgressBarHeight() const = 0;
        virtual Color getProgressBarBgColor() const = 0;
        virtual Color getProgressBarProgressColor() const = 0;
        virtual float getProgressBarIndeterminateBandWidthFactor() const = 0;
        virtual float getProgressBarIndeterminateStep() const = 0;

        virtual void drawProgressBarBackground(const Rect&) const = 0;
        virtual void drawProgressBarProgress(const Rect&, float) const = 0;
        virtual void drawProgressBarIndeterminate(const Rect&, float) const = 0;

        virtual Rect getCheckBoxCheckableArea(const Rect&) const = 0;
        virtual Extent getCheckBoxCheckableAreaPadding() const = 0;
        virtual Extent getCheckBoxCheckMarkPadding() const = 0;
        virtual Color getCheckBoxCheckableAreaBgColor() const = 0;
        virtual Color getCheckBoxCheckMarkColor() const = 0;
        virtual u_long getCheckBoxCheckDelay() const = 0;

        virtual void drawCheckBox(const char*, bool, const Rect&) const = 0;
    };

    using ThemePtr = std::shared_ptr<ITheme>;

    class DefaultTheme : public ITheme
    {
    public:
        void setGfxContext(const GfxContextPtr& gfx) final
        {
            TWM_ASSERT(gfx);
            _gfxContext = gfx;
        }

        void drawScreensaver() const final
        {
            _gfxContext->fillScreen(getScreensaverColor());
        }

        Color getScreensaverColor() const final { return 0x0000; }

        void drawDesktopBackground() const final
        {
            _gfxContext->fillRect(
                0,
                0,
                _gfxContext->width(),
                _gfxContext->height(),
                getDesktopWindowColor()
            );
        }

        Color getDesktopWindowColor() const final { return 0xb59a; }

        void setDefaultFont(const Font* font) final
        {
            _defaultFont = font;
            _gfxContext->setFont(_defaultFont);
        }

        const Font* getDefaultFont() const final { return _defaultFont; }
        void setTextSizeMultiplier(uint8_t mul) const final { _gfxContext->setTextSize(mul); }
        uint8_t getDefaultTextSizeMultiplier() const final { return 1; }

        ScreenSize getScreenSize() const final
        {
            if (_gfxContext->width() <= 320) {
                return ScreenSize::Small;
            } else if (_gfxContext->width() <= 480) {
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
                    return abs(value * 1.0f);
                case ScreenSize::Medium:
                    return abs(value * 2.0f);
                case ScreenSize::Large:
                    return abs(value * 3.0f);
            }
        }

        Extent getXPadding() const final { return abs(_gfxContext->width() * 0.07f); }
        Extent getYPadding() const final { return abs(_gfxContext->height() * 0.07f); }
        Coord getTextYOffset() const final { return getScaledValue(4); }
        Extent getWindowFrameThickness() const final { return getScaledValue(1); }
        Color getWindowTextColor() const final { return 0x0000; }
        Color getWindowBgColor() const final { return 0xdedb; }
        Color getWindowFrameColor() const final { return 0x9cf3; }
        Color getWindowFrameShadowColor() const final { return 0xb5b6; }

        Extent getDefaultButtonWidth() const final { return abs(_gfxContext->width() * 0.27f); }
        Extent getDefaultButtonHeight() const final { return abs(getDefaultButtonWidth() * 0.52f); }
        Color getButtonTextColor() const final { return 0xffff; }
        Color getButtonTextColorPressed() const final { return 0xffff; }
        Color getButtonBgColor() const final { return 0x8c71; }
        Color getButtonBgColorPressed() const final { return 0x738e; }
        Color getButtonFrameColor() const final { return 0x6b6d; }
        Color getButtonFrameColorPressed() const final { return 0x6b6d; }
        Extent getButtonLabelPadding() const final { return getScaledValue(10); }
        u_long getButtonTappedDuration() const final { return 200; }

        Coord getButtonCornerRadius() const final
        {
            static constexpr Coord radius = 4;
            switch (getScreenSize()) {
                default:
                case ScreenSize::Small:
                    return radius;
                case ScreenSize::Medium:
                    return radius + 1;
                case ScreenSize::Large:
                    return radius + 2;
            }
        }

        void drawWindowFrame(const Rect& rect, bool drawShadow = true) const final
        {
            Rect tmp = rect;
            auto pixels = getWindowFrameThickness();
            while (pixels-- > 0) {
                _gfxContext->drawRect(tmp.left, tmp.top, tmp.width(), tmp.height(),
                    getWindowFrameColor());
                tmp.deflate(1);
            }
            if (drawShadow) {
                _gfxContext->drawLine(
                    rect.left + 1,
                    rect.bottom,
                    rect.left + (rect.width() - 1),
                    rect.bottom,
                    getWindowFrameShadowColor()
                );
                _gfxContext->drawLine(
                    rect.right,
                    rect.top + 1,
                    rect.right,
                    rect.top + (rect.height() - 1),
                    getWindowFrameShadowColor()
                );
            }
        }

        void drawWindowBackground(const Rect& rect) const final
        {
            _gfxContext->fillRect(rect.left, rect.top, rect.width(), rect.height(),
                getWindowBgColor());
        }

        void drawText(const char* text, uint8_t flags, const Rect& rect,
            uint8_t textSize, Color textColor, const Font* font) const final
        {
            const bool xCenter = bitsHigh(flags, DT_CENTER);
            const bool singleLine = bitsHigh(flags, DT_SINGLE);

            uint8_t xAdv = 0, yAdv = 0, yAdvMax = 0;
            int8_t xOff = 0, yOff = 0, yOffMin = 0;
            Extent xAccum = 0;
            Extent yAccum;
            if (singleLine) {
                int16_t x, y = rect.top + (rect.height() / 2);
                uint16_t w, h;
                _gfxContext->getTextBounds(text, rect.left, y, &x, &y, &w, &h);
                yAccum = rect.top + (rect.height() / 2) + (h / 2) - 1;
            } else {
                yAccum = rect.top + getYPadding();
            }

            const Extent xPadding =
                ((singleLine && !xCenter) ? 0 : getXPadding());
            const Extent xExtent = rect.right - xPadding;
            const char* cursor = text;

            _gfxContext->setFont(font);

            while (*cursor != '\0') {
                xAccum = rect.left + xPadding;
                const char* old_cursor = cursor;
                std::deque<uint8_t> charXAdvs;
                bool clipped = false;
                while (xAccum <= xExtent && *cursor != '\0') {
                    /// TODO: handle \n and \r
                    getCharBounds(*cursor, nullptr, nullptr, &xAdv, &yAdv, &xOff,
                        &yOff, textSize, font);
                    if (xAccum + xAdv > xExtent) {
                        if (singleLine && bitsHigh(flags, DT_CLIP)) {
                            clipped = true;
                            break;
                        }
                        if (singleLine && bitsHigh(flags, DT_ELLIPSIS)) {
                            auto it = charXAdvs.rbegin();
                            if (it != charXAdvs.rend()) {
                                clipped = true;
                                xAccum -= (*it);
                                charXAdvs.pop_back();
                                cursor--;
                                break;
                            }
                        }
                    }
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
                    _gfxContext->drawChar(
                        xAccum,
                        yAccum,
                        *old_cursor++,
                        textColor,
                        textColor
# if defined(TWM_GFX_ADAFRUIT)
                        , textSize
# endif
                    );
                    xAccum += charXAdvs[
                        charXAdvs.size() - 2 - ((cursor + rewound) - old_cursor - 1)
                    ];
                }
                if (!singleLine) {
                    if (rewound > 0) { cursor++; }
                    yAccum += yAdvMax + yOffMin;
                } else {
                    if (clipped && bitsHigh(flags, DT_ELLIPSIS)) {
                        getCharBounds('.', nullptr, nullptr, &xAdv, &yAdv,
                            &xOff, &yOff, textSize, font);
                        for (uint8_t ellipsis = 0; ellipsis < 3; ellipsis++) {
                            _gfxContext->drawChar(
                                xAccum,
                                yAccum,
                                '.',
                                textColor,
                                textColor
# if defined(TWM_GFX_ADAFRUIT)
                                , textSize
# endif
                            );
                            xAccum += xAdv;
                        }
                    }
                    break;
                }
            }
        }

        void drawButtonFrame(bool pressed, const Rect& rect) const final
        {
            _gfxContext->drawRoundRect(
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
            _gfxContext->fillRoundRect(
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
            drawText(
                lbl,
                DT_SINGLE | DT_CENTER,
                rect,
                getDefaultTextSizeMultiplier(),
                pressed ? getButtonTextColorPressed() : getButtonTextColor(),
                getDefaultFont()
            );
        }

        Extent getDefaultProgressBarHeight() const final
        {
            return abs(_gfxContext->height() * 0.12f);
        }

        Color getProgressBarBgColor() const final { return 0xef5d; }
        Color getProgressBarProgressColor() const final { return 0x0ce0; }
        float getProgressBarIndeterminateBandWidthFactor() const final { return 0.33f; }

        float getProgressBarIndeterminateStep() const final
        {
            static constexpr float step = 1.0f;
            switch (getScreenSize()) {
                default:
                case ScreenSize::Small:
                    return step;
                case ScreenSize::Medium:
                    return step * 2;
                case ScreenSize::Large:
                    return step * 4;
            }
        }

        void drawProgressBarBackground(const Rect& rect) const final
        {
            _gfxContext->fillRect(rect.left, rect.top, rect.width(), rect.height(),
                getProgressBarBgColor());
        }

        void drawProgressBarProgress(const Rect& rect, float percent) const final
        {
            TWM_ASSERT(percent >= 0.0f && percent <= 100.0f);
            Rect barRect = rect;
            barRect.deflate(getWindowFrameThickness() * 2);
            float progressWidth = (barRect.width() * (min(100.0f, percent) / 100.0f));
            barRect.right = barRect.left + abs(progressWidth);
            _gfxContext->fillRect(barRect.left, barRect.top, barRect.width(),
                barRect.height(), getProgressBarProgressColor());
        }

        void drawProgressBarIndeterminate(const Rect& rect, float counter) const final
        {
            TWM_ASSERT(counter >= 0.0f && counter <= 100.0f);
            Rect barRect = rect;
            barRect.deflate(getWindowFrameThickness() * 2);
            Extent bandWidth
                = (barRect.width() * getProgressBarIndeterminateBandWidthFactor());
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
            _gfxContext->fillRect(x, barRect.top, width, barRect.height(),
                getProgressBarProgressColor());
        }

        Rect getCheckBoxCheckableArea(const Rect& rect) const final
        {
            auto checkPadding = getScaledValue(getCheckBoxCheckableAreaPadding());
            Rect checkableRect(
                rect.left,
                rect.top + checkPadding,
                rect.left + (rect.height() - (checkPadding * 2)),
                rect.top + (rect.height() - checkPadding)
            );
            checkableRect.top = rect.top + ((rect.height() / 2) - (checkableRect.height() / 2));
            return checkableRect;
        }

        Extent getCheckBoxCheckableAreaPadding() const final { return 6; }
        Extent getCheckBoxCheckMarkPadding() const final { return 4; }
        Color getCheckBoxCheckableAreaBgColor() const final { return 0xef5d; }
        Color getCheckBoxCheckMarkColor() const final { return 0x3166; }
        u_long getCheckBoxCheckDelay() const final { return 200; }

        void drawCheckBox(const char* lbl, bool checked, const Rect& rect) const final
        {
            drawWindowBackground(rect);
            Rect checkableRect = getCheckBoxCheckableArea(rect);
            _gfxContext->fillRect(
                checkableRect.left,
                checkableRect.top,
                checkableRect.width(),
                checkableRect.height(),
                getCheckBoxCheckableAreaBgColor()
            );
            drawWindowFrame(checkableRect, false);
            if (checked) {
                Rect rectCheckMark = checkableRect;
                rectCheckMark.deflate(getScaledValue(getCheckBoxCheckMarkPadding()));
                _gfxContext->fillRect(
                    rectCheckMark.left,
                    rectCheckMark.top,
                    rectCheckMark.width(),
                    rectCheckMark.height(),
                    getCheckBoxCheckMarkColor()
                );
            }
            auto checkPadding = getScaledValue(getCheckBoxCheckableAreaPadding());
            Rect textRect(
                checkableRect.right + checkPadding,
                rect.top,
                checkableRect.right + (rect.width() - checkableRect.width()),
                rect.top + rect.height()
            );
            drawText(lbl, DT_SINGLE | DT_ELLIPSIS, textRect, getDefaultTextSizeMultiplier(),
                getWindowTextColor(), getDefaultFont());
        }

    private:
        GfxContextPtr _gfxContext;
        const Font* _defaultFont = nullptr;
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
        virtual std::shared_ptr<IWindow> getChildByID(WindowID) = 0;
        virtual bool addChild(const std::shared_ptr<IWindow>&) = 0;
        virtual bool removeChildByID(WindowID) = 0;
        virtual void removeAllChildren() = 0;
        virtual void forEachChild(const std::function<bool(const std::shared_ptr<IWindow>&)>&) = 0;
        virtual void forEachChildReverse(const std::function<bool(const std::shared_ptr<IWindow>&)>&) = 0;
    };

    class IWindow : public IWindowContainer
    {
    public:
        virtual std::shared_ptr<IWindow> getParent() const = 0;
        virtual void setParent(const std::shared_ptr<IWindow>&) = 0;

        virtual Rect getRect() const noexcept = 0;
        virtual void setRect(const Rect&) noexcept = 0;

        virtual Style getStyle() const noexcept = 0;
        virtual void setStyle(Style) noexcept = 0;

        virtual WindowID getID() const noexcept = 0;

        virtual State getState() const noexcept = 0;
        virtual void setState(State) noexcept = 0;

        virtual std::string getText() const = 0;
        virtual void setText(const std::string&) = 0;

        virtual bool routeMessage(Message, MsgParam, MsgParam) = 0;
        virtual bool queueMessage(Message, MsgParam, MsgParam) = 0;
        virtual bool processQueue() = 0;

        virtual bool redraw() = 0;
        virtual bool hide() noexcept = 0;
        virtual bool show() noexcept = 0;
        virtual bool isVisible() const noexcept = 0;
        virtual bool processInput(InputParams*) = 0;
        virtual bool destroy() = 0;
        virtual bool isAlive() const noexcept = 0;

    protected:
        virtual bool onCreate(MsgParam, MsgParam) = 0;
        virtual bool onDestroy(MsgParam, MsgParam) = 0;
        virtual bool onDraw(MsgParam, MsgParam) = 0;
        virtual bool onInput(MsgParam, MsgParam) = 0;
        virtual bool onEvent(MsgParam, MsgParam) = 0;
        virtual bool onResize(MsgParam, MsgParam) = 0;

        virtual bool onTapped(Coord, Coord) = 0;
    };

    using WindowPtr          = std::shared_ptr<IWindow>;
    using WindowContainerPtr = std::shared_ptr<IWindowContainer>;

    class WindowContainer : public IWindowContainer
    {
    public:
        using WindowDeque = std::deque<WindowPtr>;

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
            for (auto child : _children) {
                if (!cb(child)) {
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
        WindowDeque _children;
# if !defined(TWM_SINGLETHREAD)
        Mutex _childMtx;
# endif
    };

    enum
    {
        WMS_SSAVER_ENABLED = 1 << 0,
        WMS_SSAVER_ACTIVE  = 1 << 1,
        WMS_SSAVER_DRAWN   = 1 << 2
    };

    class WindowManager : public std::enable_shared_from_this<WindowManager>
    {
    public:
        WindowManager() = delete;

        explicit WindowManager(
            const GfxDisplayPtr& gfxDisplay,
            const GfxContextPtr& gfxContext,
            const ThemePtr& theme,
            const Font* defaultFont
        ) : _gfxDisplay(gfxDisplay), _gfxContext(gfxContext), _theme(theme)
        {
            TWM_ASSERT(_gfxDisplay);
            TWM_ASSERT(_gfxContext);
            TWM_ASSERT(_theme);
            if (_theme && _gfxContext) {
                _theme->setGfxContext(_gfxContext);
                _theme->setDefaultFont(defaultFont);
            }
            _registry = std::make_shared<WindowContainer>();
            TWM_ASSERT(_registry);
        }

        virtual ~WindowManager()
        {
            tearDown();
        }

        void setState(State state) { _state = state; }
        State getState() const { return _state; }

        void enableScreensaver(u_long activateAfter)
        {
            _ssaverActivateAfter = activateAfter;
            _ssaverEpoch = millis();
            setState(getState() | WMS_SSAVER_ENABLED);
            TWM_LOG(TWM_DEBUG, "enabled screensaver (%lums)", activateAfter);
        }

        void disableScreensaver()
        {
            State flags = WMS_SSAVER_ENABLED | WMS_SSAVER_ACTIVE | WMS_SSAVER_DRAWN;
            setState(getState() & ~flags);
            TWM_LOG(TWM_DEBUG, "disabled screensaver");
        }

        virtual bool begin(uint8_t rotation)
        {
# if defined(TWM_GFX_ADAFRUIT)
            _gfxDisplay->begin(0);
            _gfxDisplay->setRotation(rotation);
            _gfxDisplay->setCursor(0, 0);
            return true;
# elif defined(TWM_GFX_ARDUINO)
            (void)rotation;
            return _gfxContext->begin();
# endif
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

        GfxDisplayPtr getGfxDisplay() const { return _gfxDisplay; }
        GfxContextPtr getGfxContext() const { return _gfxContext; }
        ThemePtr getTheme() const { return _theme; }

        Extent getScreenWidth() const { return _gfxContext->width(); }
        Extent getScreenHeight() const { return _gfxContext->height(); }

        template<class TWindow>
        std::shared_ptr<TWindow> createWindow(
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
                TWM_LOG(TWM_ERROR, "OOM");
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
        std::shared_ptr<TPrompt> createPrompt(
            const WindowPtr& parent,
            WindowID id,
            Style style,
            const std::string& text,
            const std::deque<typename TPrompt::ButtonInfo>& buttons,
            const typename TPrompt::ResultCallback& callback
        )
        {
            TWM_ASSERT(bitsHigh(style, STY_PROMPT));
            auto prompt = createWindow<TPrompt>(
                parent,
                id,
                style,
                _theme->getXPadding(),
                _theme->getYPadding(),
                getScreenWidth() - (_theme->getXPadding() * 2),
                getScreenHeight() - (_theme->getYPadding() * 2),
                text,
                [&](const std::shared_ptr<TPrompt>& win)
                {
                    for (const auto& btn : buttons) {
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
        std::shared_ptr<TBar> createProgressBar(
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
            if (bitsHigh(getState(), WMS_SSAVER_ENABLED)) {
                _ssaverEpoch = millis();
                if (bitsHigh(getState(), WMS_SSAVER_ACTIVE)) {
                    return;
                }
            }
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

        virtual void update()
        {
            if (bitsHigh(getState(), WMS_SSAVER_ENABLED)) {
                if (millis() - _ssaverEpoch >= _ssaverActivateAfter) {
                    if (!bitsHigh(getState(), WMS_SSAVER_ACTIVE)) {
                        setState(getState() | WMS_SSAVER_ACTIVE);
                        TWM_LOG(TWM_DEBUG, "activated screensaver");
                    }
                } else {
                    if (bitsHigh(getState(), WMS_SSAVER_ACTIVE)) {
                        setState(getState() & ~(WMS_SSAVER_ACTIVE | WMS_SSAVER_DRAWN));
                        TWM_LOG(TWM_DEBUG, "de-activated screensaver");
                    }
                }
            }
            if (bitsHigh(getState(), WMS_SSAVER_ACTIVE)) {
                if (!bitsHigh(getState(), WMS_SSAVER_DRAWN)) {
                    _theme->drawScreensaver();
                    setState(getState() | WMS_SSAVER_DRAWN);
                }
            } else {
                _theme->drawDesktopBackground();
                _registry->forEachChild([](const WindowPtr& win)
                {
                    win->processQueue();
                    // TODO: is this window completely covered by another, or off the screen?
                    win->redraw();
                    return true;
                });
            }
        }

        void render()
        {
            /// TODO: implement different calls for different color modes
# if defined(TWM_GFX_ADAFRUIT)
            _gfxDisplay->drawRGBBitmap(0, 0, _gfxContext->getBuffer(),
                _gfxContext->width(), _gfxContext->height());
# elif defined(TWM_GFX_ARDUINO)
            _gfxContext->flush();
# endif
        }

    protected:
        WindowContainerPtr _registry;
        GfxDisplayPtr _gfxDisplay;
        GfxContextPtr _gfxContext;
        ThemePtr _theme;
        State _state = 0;
        u_long _ssaverEpoch = 0UL;
        u_long _ssaverActivateAfter = 0UL;
    };

    using WindowManagerPtr = std::shared_ptr<WindowManager>;

    template<class TTheme, class TGfxDisplay>
    WindowManagerPtr createWindowManager(
        const std::shared_ptr<TGfxDisplay>& display,
        const std::shared_ptr<IGfxContext>& context,
        const std::shared_ptr<TTheme>& theme,
        const Font* defaultFont
    )
    {
        static_assert(std::is_base_of<IGfxDisplay, TGfxDisplay>::value);
        static_assert(std::is_base_of<ITheme, TTheme>::value);
        return std::make_shared<WindowManager>(display, context, theme, defaultFont);
    }

    class Window : public IWindow, public std::enable_shared_from_this<IWindow>
    {
    public:
        Window() = default;

        Window(
            const WindowManagerPtr& wm,
            const WindowPtr& parent,
            WindowID id,
            Style style,
            const Rect& rect,
            const std::string& text
        ) : _wm(wm), _parent(parent), _rect(rect), _style(style), _id(id), _text(text)
        {
        }

        virtual ~Window() = default;

        bool hasChildren() override { return _children.hasChildren(); }
        size_t childCount() override { return _children.childCount(); }
        WindowPtr getChildByID(WindowID id) override { return _children.getChildByID(id); }
        bool addChild(const WindowPtr& child) override { return _children.addChild(child); }
        bool removeChildByID(WindowID id) override { return _children.removeChildByID(id); }
        void removeAllChildren() override { return _children.removeAllChildren(); }

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
                case MSG_CREATE: return onCreate(p1, p2);
                case MSG_DESTROY: return onDestroy(p1, p2);
                case MSG_DRAW: {
                    auto parent = getParent();
                    if (!isVisible() || !isAlive() ||
                        (parent && (!parent->isVisible() || !parent->isAlive()))) {
                        return false;
                    }
                    return onDraw(p1, p2);
                }
                case MSG_INPUT: return onInput(p1, p2);
                case MSG_EVENT: return onEvent(p1, p2);
                case MSG_RESIZE: return onResize(p1, p2);
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
            return msg == MSG_INPUT && getMsgParamLoWord(p1) == INPUT_TAP;
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
        /* ====== Begin message handlers ====== */

        /** MSG_CREATE: p1 = 0, p2 = 0. */
        bool onCreate(MsgParam p1, MsgParam p2) override { return true; }

        /** MSG_DESTROY: p1 = 0, p2 = 0. */
        bool onDestroy(MsgParam p1, MsgParam p2) override
        {
            setState(getState() & ~STA_ALIVE);
            return true;
        }

        /** MSG_DRAW: p1 = 0, p2 = 0. */
        bool onDraw(MsgParam p1, MsgParam p2) override
        {
            auto theme = _getTheme();
            if (theme) {
                Rect rect = getRect();
                TWM_ASSERT(rect.width() > 0 && rect.height() > 0);
                theme->drawWindowBackground(rect);
                theme->drawWindowFrame(rect, true);
                return true;
            }
            return false;
        }

        /** MSG_INPUT: p1 = (loword: type), p2 = (hiword: x, loword: y).
         * Returns true if the input event was consumed by this window, false otherwise. */
        bool onInput(MsgParam p1, MsgParam p2) override
        {
            InputParams params;
            params.type = getMsgParamLoWord(p1);
            params.x    = getMsgParamHiWord(p2);
            params.y    = getMsgParamLoWord(p2);

            switch (params.type) {
                case INPUT_TAP: return onTapped(params.x, params.y);
                default:
                    TWM_ASSERT(false);
                break;
            }
            return false;
        }

        /** MSG_EVENT: p1 = EventType, p2 = child WindowID. */
        bool onEvent(MsgParam p1, MsgParam p2) override { return true; }

        /** MSG_RESIZE: p1 = 0, p2 = 0. */
        bool onResize(MsgParam p1, MsgParam p2) override
        {
            TWM_ASSERT(bitsHigh(getStyle(), STY_AUTOSIZE));
            return false;
        }

        /* ====== End message handlers ====== */

        bool onTapped(Coord x, Coord y) override { return false; }

        WindowManagerPtr _getWM() const { return _wm; }

        GfxContextPtr _getGfxContext() const
        {
            auto wm = _getWM();
            return wm ? wm->getGfxContext() : nullptr;
        }

        ThemePtr _getTheme() const
        {
            auto wm = _getWM();
            return wm ? wm->getTheme() : nullptr;
        }

    protected:
        WindowContainer _children;
        PackagedMessageQueue _queue;
# if !defined(TWM_SINGLETHREAD)
        Mutex _queueMtx;
# endif
        WindowManagerPtr _wm;
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
                parent->queueMessage(MSG_EVENT, EVT_CHILD_TAPPED, getID());
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
            auto gfx = _getGfxContext();
            auto theme = _getTheme();
            if (gfx && theme) {
                Coord x, y;
                Extent width, height;
                Rect rect = getRect();
                gfx->getTextBounds(getText().c_str(), rect.left, rect.top, &x, &y, &width, &height);
                rect.right = rect.left + max(width, theme->getDefaultButtonWidth()) + (theme->getButtonLabelPadding() * 2);
                rect.bottom = rect.top + theme->getDefaultButtonHeight();
                setRect(rect);
                /// TODO: if not autosize, clip label, perhaps with ellipsis.
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
        TWM_CONST(uint8_t, DrawTextFlags, DT_SINGLE | DT_ELLIPSIS);

        using Window::Window;
        Label() = default;
        virtual ~Label() = default;

        bool onDraw(MsgParam p1, MsgParam p2) override
        {
            auto theme = _getTheme();
            if (theme) {
                Rect rect = getRect();
                theme->drawWindowBackground(rect);
                theme->drawText(
                    getText().c_str(),
                    DrawTextFlags,
                    rect,
                    theme->getDefaultTextSizeMultiplier(),
                    theme->getWindowTextColor(),
                    theme->getDefaultFont()
                );
                return true;
            }
            return false;
        }
    };

    class MultilineLabel : public Window
    {
    public:
        TWM_CONST(uint8_t, DrawTextFlags, DT_CENTER);

        using Window::Window;
        MultilineLabel() = default;
        virtual ~MultilineLabel() = default;

        bool onDraw(MsgParam p1, MsgParam p2) override
        {
            auto theme = _getTheme();
            if (theme) {
                Rect rect = getRect();
                theme->drawWindowBackground(rect);
                theme->drawText(
                    getText().c_str(),
                    DrawTextFlags,
                    rect,
                    theme->getDefaultTextSizeMultiplier(),
                    theme->getWindowTextColor(),
                    theme->getDefaultFont()
                );
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

        bool onCreate(MsgParam p1, MsgParam p2) override
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
                        rect.left + theme->getXPadding(),
                        rect.top + theme->getYPadding(),
                        rect.right - rect.left - (theme->getXPadding() * 2),
                        rect.bottom - rect.top - ((theme->getYPadding() * 3) + theme->getDefaultButtonHeight()),
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
                        rectBtn.top = rectLbl.bottom + theme->getYPadding();
                        rectBtn.bottom = rectBtn.top + theme->getDefaultButtonHeight();
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
                                    rectBtn.left = rect.left + theme->getXPadding();
                                }
                                break;
                                default:
                                    TWM_ASSERT(false);
                                return false;
                            }
                            rectBtn.right = rectBtn.left + width;
                        } else {
                            rectBtn.right = rect.right - theme->getXPadding();
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

        bool onEvent(MsgParam p1, MsgParam p2) override
        {
            switch (static_cast<EventType>(p1)) {
                case EVT_CHILD_TAPPED:
                    hide();
                    if (_callback) {
                        _callback(static_cast<WindowID>(p2));
                    }
                return true;
                default:
                    TWM_ASSERT(false);
                break;
            }
            return false;
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

        Style getProgressBarStyle() const noexcept { return _barStyle; }
        void setProgressBarStyle(Style pbarStyle) noexcept { _barStyle = pbarStyle; }

        float getProgressValue() const noexcept { return _value; }
        void setProgressValue(float value) noexcept { _value = value; }

    protected:
        bool onDraw(MsgParam p1, MsgParam p2) override
        {
            auto theme = _getTheme();
            if (theme) {
                Rect rect = getRect();
                theme->drawProgressBarBackground(rect);
                theme->drawWindowFrame(rect, false);
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

        Style _barStyle = 0;
        float _value = 0;
    };

    class CheckBox : public Window
    {
    public:
        using Window::Window;
        CheckBox() = default;
        virtual ~CheckBox() = default;

        void setChecked(bool checked)
        {
            if (isChecked() != checked) {
                _lastToggle = millis();
                if (checked) {
                    setState(getState() | STA_CHECKED);
                } else {
                    setState(getState() & ~STA_CHECKED);
                }
                redraw();
            }
        }

        bool isChecked() const noexcept { return bitsHigh(getState(), STA_CHECKED); }

    protected:
        bool onDraw(MsgParam p1, MsgParam p2) override
        {
            auto theme = _getTheme();
            if (theme) {
                Rect rect = getRect();
                theme->drawCheckBox(getText().c_str(), isChecked(), rect);
                return true;
            }
            return false;
        }

        bool onTapped(Coord x, Coord y) override
        {
            auto theme = _getTheme();
            if (theme) {
                if (millis() - _lastToggle >= theme->getCheckBoxCheckDelay()) {
                    setChecked(!isChecked());
                    return true;
                }
            }
            return false;
        }

    private:
        u_long _lastToggle = 0UL;
    };
} // namespace thumby

#endif // !_THUMBY_WM_H_INCLUDED
