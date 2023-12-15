/*
 * twm.h
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

// Disables mutex locks used in multi-threaded environments.
# define TWM_SINGLETHREAD

// Enables serial logging (increases compiled binary size substantially!).
# define TWM_ENABLE_LOGGING

// Enables internal diagnostics; implies TWM_ENABLE_LOGGING.

# if defined (TWM_DIAGNOSTICS)
#  include <typeinfo>
#  include <cxxabi.h>
# endif

# if defined(TWM_ENABLE_LOGGING) || defined(TWM_DIAGNOSTICS)
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
            case TWM_WARN:  prefix = 'W'; break; \
            case TWM_DEBUG: prefix = 'D'; break; \
        } \
        Serial.printf("[%c] (%s:%d): ", prefix, basename(__FILE__), __LINE__); \
        Serial.printf(fmt "\n" __VA_OPT__(,) __VA_ARGS__); \
    } while(false)
# define TWM_BACKTRACE_FRAMES 5
# if defined(ESP32) || defined(ESP8266)
#  include <esp_debug_helpers.h>
#  define print_backtrace() \
    ets_install_putc1([](char c) \
    { \
        Serial.print(c); \
    }); \
    esp_backtrace_print(TWM_BACKTRACE_FRAMES)
# else
#  define print_backtrace
# endif
#  define TWM_ASSERT(expr) \
    do { \
        if (!(expr)) { \
            TWM_LOG(TWM_ERROR, "!!! ASSERT: '" #expr "'"); \
            print_backtrace(); \
            while (true); \
        } \
    } while (false)
# else
#  define TWM_LOG(lvl, fmt, ...)
#  define TWM_ASSERT(expr)
# endif

# define TWM_UNUSED(var) (void)(var);
# define TWM_BOOL2STR(b) ((b) ? "true" : "false")

# define TWM_CONST(type, name, value) \
    static constexpr PROGMEM type name = value

# if defined(TWM_GFX_ADAFRUIT) && __has_include(<Adafruit_GFX.h>)
#  include <Adafruit_GFX.h>
#  if defined(_ADAFRUIT_RA8875_H)
    using IGfxDisplay = Adafruit_RA8875;
#  elif __has_include(<Adafruit_SPITFT.h>)
#   include <Adafruit_SPITFT.h>
    using IGfxDisplay = Adafruit_SPITFT;
#  else
    using IGfxDisplay = Adafruit_GFX;
#  endif
    using IGfxContext16 = GFXcanvas16;
#   if defined(__AVR__)
#    include <avr/pgmspace.h>
#   elif defined(ESP32) || defined(ESP8266)
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
#   error "required Arduino_Canvas implementation unavailable due to LITTLE_FOOT_PRINT"
#  endif
#  if defined(ATTINY_CORE)
#   error "required GFXfont implementation unavailable due to ATTINY_CORE"
#  endif
    using IGfxContext16 = Arduino_Canvas;
# else
#  error "define TWM_GFX_ADAFRUIT or TWM_GFX_ARDUINO, and install the relevant \
library in order to select a low-level graphics driver"
# endif

namespace thumby
{
    /** Window identifier. */
    using WindowID = uint8_t;

    /** Represents an invalid window identifier. */
    TWM_CONST(WindowID, WID_INVALID, 0);

    /** Window style bitmask. */
    using Style = uint16_t;

    /** State bitmask. */
    using State = uint16_t;

    /** Window message parameter type. */
    using MsgParam = uint32_t;

    /** Window message parameter component type. */
    using MsgParamWord = uint16_t;

    /** Physical display driver. */
    using GfxDisplay = IGfxDisplay;

    /** Pointer to physical display driver. */
    using GfxDisplayPtr = std::shared_ptr<GfxDisplay>;

# if defined(TWM_COLOR_565)
    using Color      = uint16_t;      /**< Color type (16-bit 565 RGB). */
#  if !defined(ADAFRUIT_RA8875)
    using GfxContext = IGfxContext16; /**< Graphics context (16-bit 565 RGB). */
#  else
    using GfxContext = Adafruit_RA8875;
#  endif
# elif defined(TWM_COLOR_888)
#  error "24-bit RGB mode is not yet implemented"
# else
#  error "define TWM_COLOR_565 or TWM_COLOR_888 in order to select a color mode"
# endif

    /** Pointer to graphics context (e.g. canvas/frame buffer). */
    using GfxContextPtr = std::shared_ptr<GfxContext>;

    /** Font type. */
    using Font = GFXfont;

    /** Coordinate in 3D space (e.g. X, Y, or Z). */
    using Coord = int16_t;

    /** Extent (e.g. width, height). */
    using Extent = uint16_t;

    /** Point in 2D space. */
    struct Point
    {
        Point() = default;

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

        explicit Rect(Coord l, Coord t, Extent r, Extent b)
            : left(l), top(t), right(r), bottom(b)
        {
        }

        bool operator==(const Rect& rhs) const noexcept
        {
            return left   == rhs.left &&
                   top    == rhs.top &&
                   right  == rhs.right &&
                   bottom == rhs.bottom;
        }

        bool operator !=(const Rect& rhs) const noexcept
        {
            return !(*this == rhs);
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
            TWM_ASSERT(px < width());
            TWM_ASSERT(px < height());
            left   += px;
            top    += px;
            right  -= px;
            bottom -= px;
        }

        void mergeRect(const Rect& other) noexcept
        {
            if (other.left < left) {
                left = other.left;
            }
            if (other.top < top) {
                top = other.top;
            }
            if (other.right > right) {
                right = other.right;
            }
            if (other.bottom > bottom) {
                bottom = other.bottom;
            }
        }

        bool overlapsRect(const Rect& other) const noexcept
        {
            return other.pointWithin(left, top) ||
                   other.pointWithin(right, top) ||
                   other.pointWithin(left, bottom) ||
                   other.pointWithin(right, bottom);
        }

        bool intersectsRect(const Rect& other) const noexcept
        {
            return overlapsRect(other) || other.overlapsRect(*this);
        }

        bool outsideRect(const Rect& other) const noexcept
        {
            return !other.pointWithin(left, top) &&
                   !other.pointWithin(right, top) &&
                   !other.pointWithin(left, bottom) &&
                   !other.pointWithin(right, bottom);
        }

        bool withinRect(const Rect& other) const noexcept
        {
            return other.pointWithin(left, top) &&
                   other.pointWithin(right, top) &&
                   other.pointWithin(left, bottom) &&
                   other.pointWithin(right, bottom);
        }

        bool pointWithin(Coord x, Coord y) const noexcept
        {
            return x >= left && x <= right && y >= top && y <= bottom;
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
        const GFXfont* font = nullptr) noexcept
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
    inline bool bitsHigh(const T1& bitmask, const T2& bits) noexcept
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
        STY_VISIBLE    =  1 << 0,
        STY_CHILD      =  1 << 1,
        STY_FRAME      =  1 << 2,
        STY_SHADOW     =  1 << 3,
        STY_TOPLEVEL   = (1 << 4) | STY_FRAME | STY_SHADOW,
        STY_AUTOSIZE   =  1 << 5,
        STY_FULLSCREEN =  1 << 6,
        STY_BUTTON     =  1 << 7,
        STY_LABEL      =  1 << 8,
        STY_PROMPT     =  (1 << 9) | STY_TOPLEVEL,
        STY_PROGBAR    =  1 << 10,
        STY_CHECKBOX   =  1 << 11
    };

    enum
    {
        STA_ALIVE   = 1 << 0, /**< Active (not yet destroyed). */
        STA_CHECKED = 1 << 1, /**< Checked/highlighted item. */
        STA_DIRTY   = 1 << 2  /**< Needs redrawing. */
    };

    enum
    {
        PBR_NORMAL        = 1 << 0, /**< Standard linear-fill progress bar. */
        PBR_INDETERMINATE = 1 << 1  /**< Marquee-style progress bar. */
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

    typedef enum
    {
        INPUT_TAP = 1
    } InputType;

    struct InputParams
    {
        WindowID handledBy = WID_INVALID;
        InputType type = InputType(0);
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

    typedef enum
    {
        COLOR_SCREENSAVER = 1,

        COLOR_PROMPT_BG,
        COLOR_PROMPT_FRAME,
        COLOR_PROMPT_SHADOW,

        COLOR_WINDOW_TEXT,
        COLOR_WINDOW_BG,
        COLOR_WINDOW_FRAME,
        COLOR_WINDOW_SHADOW,

        COLOR_BUTTON_TEXT,
        COLOR_BUTTON_TEXT_PRESSED,
        COLOR_BUTTON_BG,
        COLOR_BUTTON_BG_PRESSED,
        COLOR_BUTTON_FRAME,
        COLOR_BUTTON_FRAME_PRESSED,

        COLOR_PROGRESS_BG,
        COLOR_PROGRESS_FILL,

        COLOR_CHECKBOX_CHECK_BG,
        COLOR_CHECKBOX_CHECK_FRAME,
        COLOR_CHECKBOX_CHECK
    } ColorID;

    typedef enum
    {
        METRIC_X_PADDING = 1,               /**< Extent */
        METRIC_Y_PADDING,                   /**< Extent */

        METRIC_DEF_TEXT_SIZE,               /**< uint8_t */

        METRIC_WINDOW_FRAME_PX,             /**< Extent */
        METRIC_CORNER_RADIUS_WINDOW,        /**< Coord */
        METRIC_CORNER_RADIUS_BUTTON,        /**< Coord */
        METRIC_CORNER_RADIUS_PROMPT,        /**< Coord */
        METRIC_CORNER_RADIUS_CHECKBOX,      /**< Coord */

        METRIC_DEF_BUTTON_CX,               /**< Extent */
        METRIC_DEF_BUTTON_CY,               /**< Extent */
        METRIC_BUTTON_LABEL_PADDING,        /**< Extent */
        METRIC_BUTTON_TAPPED_DURATION,      /**< uint32_t */

        METRIC_MAX_PROMPT_CX,               /**< Extent */
        METRIC_MAX_PROMPT_CY,               /**< Extent */

        METRIC_DEF_PROGBAR_HEIGHT,          /**< Extent */
        METRIC_PROGBAR_MARQUEE_CX_FACTOR,   /**< float */
        METRIC_PROGBAR_MARQUEE_STEP,        /**< float */

        METRIC_DEF_CHECKBOX_HEIGHT,         /**< Extent */
        METRIC_CHECKBOX_CHECK_AREA_PADDING, /**< Extent */
        METRIC_CHECKBOX_CHECK_MARK_PADDING, /**< Extent */
        METRIC_CHECKBOX_CHECK_DELAY         /**< uint32_t */
    } MetricID;

    struct Variant
    {
        enum
        {
            EMPTY  = 0,
            EXTENT = 1,
            COORD  = 2,
            UINT8  = 3,
            UINT32 = 4,
            FLOAT  = 5
        };

        Variant() = default;

        int getType() const noexcept { return _type; }

        Extent getExtent() const noexcept
        {
            TWM_ASSERT(_type == EXTENT);
            return _extentValue;
        }

        void setExtent(const Extent& extent) noexcept
        {
            _extentValue = extent;
            _type = EXTENT;
        }

        Coord getCoord() const noexcept
        {
            TWM_ASSERT(_type == COORD);
            return _coordValue;
        }

        void setCoord(const Coord& coord) noexcept
        {
            _coordValue = coord;
            _type = COORD;
        }

        uint8_t getUint8() const noexcept
        {
            TWM_ASSERT(_type == UINT8);
            return _uint8Value;
        }

        void setUint8(const uint8_t& octet) noexcept
        {
            _uint8Value = octet;
            _type = UINT8;
        }

        uint32_t getUint32() const noexcept
        {
            TWM_ASSERT(_type == UINT32);
            return _uint32Value;
        }

        void setUint32(const uint32_t& dword) noexcept
        {
            _uint32Value = dword;
            _type = UINT32;
        }

        float getFloat() const noexcept
        {
            TWM_ASSERT(_type == FLOAT);
            return _floatValue;
        }

        void setFloat(const float& flt) noexcept
        {
            _floatValue = flt;
            _type = FLOAT;
        }

    private:
        union
        {
            Extent _extentValue;
            Coord _coordValue;
            uint8_t _uint8Value;
            uint32_t _uint32Value;
            float _floatValue = 0.0f;
        };
        int _type = EMPTY;
    };

    class ITheme
    {
    public:
        enum DisplaySize
        {
            Small = 0,
            Medium,
            Large
        };

        virtual void setGfxContext(const GfxContextPtr&) = 0;

        virtual Color getColor(ColorID) const = 0;
        virtual Variant getMetric(MetricID) const = 0;

        virtual void drawScreensaver() const = 0;

        virtual void setDefaultFont(const Font*) = 0;
        virtual const Font* getDefaultFont() const = 0;
        virtual void setTextSize(uint8_t) const = 0;

        virtual DisplaySize getDisplaySize() const = 0;
        virtual Extent getScaledValue(Extent) const = 0;

        virtual void drawWindowFrame(const Rect&, Coord, Color) const = 0;
        virtual void drawWindowShadow(const Rect&, Coord, Color) const = 0;
        virtual void drawWindowBackground(const Rect&, Coord, Color) const = 0;
        virtual void drawText(const char*, uint8_t, const Rect&,
            uint8_t, Color, const Font*) const = 0;

        virtual void drawProgressBarBackground(const Rect&) const = 0;
        virtual void drawProgressBarProgress(const Rect&, float) const = 0;
        virtual void drawProgressBarIndeterminate(const Rect&, float) const = 0;

        virtual void drawCheckBox(const char*, bool, const Rect&) const = 0;
    };

    using ThemePtr = std::shared_ptr<ITheme>;

    class DefaultTheme : public ITheme
    {
    public:
        DefaultTheme() = default;

        void setGfxContext(const GfxContextPtr& gfx) final
        {
            TWM_ASSERT(gfx);
            _gfxContext = gfx;
        }

        Color getColor(ColorID colorID) const final
        {
            switch (colorID) {
                case COLOR_SCREENSAVER:          return 0x0000;
                case COLOR_PROMPT_BG:            return 0xef5c;
                case COLOR_PROMPT_FRAME:         return 0x9cf3;
                case COLOR_PROMPT_SHADOW:        return 0xb5b6;
                case COLOR_WINDOW_TEXT:          return 0x0000;
                case COLOR_WINDOW_BG:            return 0xdedb;
                case COLOR_WINDOW_FRAME:         return 0x9cf3;
                case COLOR_WINDOW_SHADOW:        return 0xb5b6;
                case COLOR_BUTTON_TEXT:          return 0xffff;
                case COLOR_BUTTON_TEXT_PRESSED:  return 0xffff;
                case COLOR_BUTTON_BG:            return 0x8c71;
                case COLOR_BUTTON_BG_PRESSED:    return 0x738e;
                case COLOR_BUTTON_FRAME:         return 0x6b6d;
                case COLOR_BUTTON_FRAME_PRESSED: return 0x6b6d;
                case COLOR_PROGRESS_BG:          return 0xef5d;
                case COLOR_PROGRESS_FILL:        return 0x0ce0;
                case COLOR_CHECKBOX_CHECK_BG:    return 0xef5d;
                case COLOR_CHECKBOX_CHECK:       return 0x3166;
                case COLOR_CHECKBOX_CHECK_FRAME: return 0x9cf3;
                default:
                    TWM_ASSERT("!invalid color ID");
                    return Color(0);
            }
        }

        Variant getMetric(MetricID metricID) const final
        {
            Variant retval;
            switch (metricID) {
                case METRIC_X_PADDING:
                    retval.setExtent(abs(_gfxContext->width() * 0.05f));
                break;
                case METRIC_Y_PADDING:
                    retval.setExtent(abs(_gfxContext->height() * 0.05f));
                break;
                case METRIC_DEF_TEXT_SIZE:
                    retval.setUint8(1);
                break;
                case METRIC_WINDOW_FRAME_PX:
                    retval.setExtent(1);
                break;
                case METRIC_CORNER_RADIUS_WINDOW:
                    retval.setCoord(0);
                break;
                case METRIC_CORNER_RADIUS_BUTTON:
                    retval.setCoord(getScaledValue(4));
                break;
                case METRIC_CORNER_RADIUS_PROMPT:
                    retval.setCoord(getScaledValue(4));
                break;
                case METRIC_CORNER_RADIUS_CHECKBOX:
                    retval.setCoord(getScaledValue(0));
                break;
                case METRIC_DEF_BUTTON_CX:
                    retval.setExtent(abs(max(_gfxContext->width() * 0.19f, 60.0f)));
                break;
                case METRIC_DEF_BUTTON_CY: {
                    const auto btnWidth = getMetric(METRIC_DEF_BUTTON_CX).getExtent();
                    retval.setExtent(abs(btnWidth * 0.52f));
                }
                break;
                case METRIC_BUTTON_LABEL_PADDING:
                    retval.setExtent(getScaledValue(10));
                break;
                case METRIC_BUTTON_TAPPED_DURATION:
                    retval.setUint32(200);
                break;
                case METRIC_MAX_PROMPT_CX:
                    retval.setExtent(abs(_gfxContext->width() * 0.75f));
                break;
                case METRIC_MAX_PROMPT_CY:
                    retval.setExtent(abs(_gfxContext->height() * 0.75f));
                break;
                case METRIC_DEF_PROGBAR_HEIGHT:
                    retval.setExtent(abs(_gfxContext->height() * 0.10f));
                break;
                case METRIC_PROGBAR_MARQUEE_CX_FACTOR:
                    retval.setFloat(0.33f);
                break;
                case METRIC_PROGBAR_MARQUEE_STEP: {
                    static constexpr float step = 1.0f;
                    switch (getDisplaySize()) {
                        default:
                        case DisplaySize::Small:
                            retval.setFloat(step);
                        break;
                        case DisplaySize::Medium:
                            retval.setFloat(step * 2.0f);
                        break;
                        case DisplaySize::Large:
                            retval.setFloat(step * 4.0f);
                        break;
                    }
                }
                break;
                case METRIC_DEF_CHECKBOX_HEIGHT:
                    retval.setExtent(abs(_gfxContext->height() * 0.10f));
                break;
                case METRIC_CHECKBOX_CHECK_AREA_PADDING:
                    retval.setExtent(getScaledValue(2));
                break;
                case METRIC_CHECKBOX_CHECK_MARK_PADDING:
                    retval.setExtent(getScaledValue(2));
                break;
                case METRIC_CHECKBOX_CHECK_DELAY:
                    retval.setUint32(200);
                break;
                default:
                    TWM_ASSERT(!"invalid metric ID");
                break;
            }
            return retval;
        }

        void drawScreensaver() const final
        {
            _gfxContext->fillScreen(getColor(COLOR_SCREENSAVER));
        }

        void setDefaultFont(const Font* font) final
        {
            _defaultFont = font;
            _gfxContext->setFont(_defaultFont);
        }

        const Font* getDefaultFont() const final { return _defaultFont; }
        void setTextSize(uint8_t size) const final { _gfxContext->setTextSize(size); }

        DisplaySize getDisplaySize() const final
        {
            const auto width  = _gfxContext->width();
            const auto height = _gfxContext->height();
            if (width <= 320 && height <= 320) {
                return DisplaySize::Small;
            } else if (width <= 480 && height <= 480) {
                return DisplaySize::Medium;
            } else {
                return DisplaySize::Large;
            }
        }

        Extent getScaledValue(Extent value) const final
        {
            switch (getDisplaySize()) {
                default:
                case DisplaySize::Small:
                    return abs(value * 1.0f);
                case DisplaySize::Medium:
                    return abs(value * 2.0f);
                case DisplaySize::Large:
                    return abs(value * 3.0f);
            }
        }

        void drawWindowFrame(const Rect& rect, Coord radius, Color color) const final
        {
            auto tmp = rect;
            auto pixels = getMetric(METRIC_WINDOW_FRAME_PX).getExtent();
            while (pixels-- > 0) {
                _gfxContext->drawRoundRect(tmp.left, tmp.top, tmp.width(), tmp.height(),
                    radius, color);
                tmp.deflate(1);
            }
        }

        void drawWindowShadow(const Rect& rect, Coord radius, Color color) const final
        {
            const auto thickness = getMetric(METRIC_WINDOW_FRAME_PX).getExtent();
            _gfxContext->drawLine(
                rect.left + radius + thickness,
                rect.bottom,
                rect.left + (rect.width() - (radius + (thickness * 2))),
                rect.bottom,
                color
            );
            _gfxContext->drawLine(
                rect.right,
                rect.top + radius + thickness,
                rect.right,
                rect.top + (rect.height() - (radius + (thickness * 2))),
                color
            );
        }

        void drawWindowBackground(const Rect& rect, Coord radius, Color color) const final
        {
            _gfxContext->fillRoundRect(rect.left, rect.top, rect.width(), rect.height(),
                radius, color);
        }

        void drawText(const char* text, uint8_t flags, const Rect& rect,
            uint8_t textSize, Color textColor, const Font* font) const final
        {
            _gfxContext->setTextSize(textSize);
            _gfxContext->setFont(font);

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
                yAccum = rect.top + getMetric(METRIC_Y_PADDING).getExtent();
            }

            const Extent xPadding =
                ((singleLine && !xCenter) ? 0 : getMetric(METRIC_X_PADDING).getExtent());
            const Extent xExtent = rect.right - xPadding;
            const char* cursor = text;

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

        void drawProgressBarBackground(const Rect& rect) const final
        {
            _gfxContext->fillRect(rect.left, rect.top, rect.width(), rect.height(),
                getColor(COLOR_PROGRESS_BG));
        }

        void drawProgressBarProgress(const Rect& rect, float percent) const final
        {
            TWM_ASSERT(percent >= 0.0f && percent <= 100.0f);
            auto barRect = rect;
            barRect.deflate(getMetric(METRIC_WINDOW_FRAME_PX).getExtent() * 2);
            barRect.right = barRect.left + abs(barRect.width() * (min(100.0f, percent) / 100.0f));
            _gfxContext->fillRect(barRect.left, barRect.top, barRect.width(),
                barRect.height(), getColor(COLOR_PROGRESS_FILL));
        }

        void drawProgressBarIndeterminate(const Rect& rect, float counter) const final
        {
            TWM_ASSERT(counter >= 0.0f && counter <= 100.0f);
            auto barRect = rect;
            barRect.deflate(getMetric(METRIC_WINDOW_FRAME_PX).getExtent() * 2);
            Extent marqueeWidth
                = (barRect.width() * getMetric(METRIC_PROGBAR_MARQUEE_CX_FACTOR).getFloat());
            Coord offset
                = (barRect.width() + marqueeWidth) * (min(100.0f, counter) / 100.0f);
            static Coord reverseOffset = marqueeWidth;
            Coord x = 0;
            Extent width = 0;
            if (offset < marqueeWidth) {
                x = barRect.left;
                if (counter <= __FLT_EPSILON__) {
                    reverseOffset = marqueeWidth;
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
                    marqueeWidth,
                    static_cast<Extent>(barRect.right - x)
                );
            }
            _gfxContext->fillRect(x, barRect.top, width, barRect.height(),
                getColor(COLOR_PROGRESS_FILL));
        }

        void drawCheckBox(const char* lbl, bool checked, const Rect& rect) const final
        {
            const auto radius = getMetric(METRIC_CORNER_RADIUS_CHECKBOX).getCoord();
            drawWindowBackground(rect, radius, getColor(COLOR_WINDOW_BG));
            auto checkableRect = Rect(
                rect.left,
                rect.top + getMetric(METRIC_CHECKBOX_CHECK_AREA_PADDING).getExtent(),
                rect.left + (rect.height() - (getMetric(METRIC_CHECKBOX_CHECK_AREA_PADDING).getExtent() * 2)),
                rect.top + (rect.height() - getMetric(METRIC_CHECKBOX_CHECK_AREA_PADDING).getExtent())
            );
            checkableRect.top = rect.top + ((rect.height() / 2) - (checkableRect.height() / 2));
            _gfxContext->fillRoundRect(
                checkableRect.left,
                checkableRect.top,
                checkableRect.width(),
                checkableRect.height(),
                radius,
                getColor(COLOR_CHECKBOX_CHECK_BG)
            );
            drawWindowFrame(checkableRect, radius, getColor(COLOR_CHECKBOX_CHECK_FRAME));
            if (checked) {
                auto rectCheckMark = checkableRect;
                rectCheckMark.deflate(getMetric(METRIC_CHECKBOX_CHECK_MARK_PADDING).getExtent());
                _gfxContext->fillRoundRect(
                    rectCheckMark.left,
                    rectCheckMark.top,
                    rectCheckMark.width(),
                    rectCheckMark.height(),
                    radius,
                    getColor(COLOR_CHECKBOX_CHECK)
                );
            }
            Rect textRect(
                checkableRect.right + (getMetric(METRIC_CHECKBOX_CHECK_MARK_PADDING).getExtent() * 2),
                rect.top,
                checkableRect.right + (rect.width() - checkableRect.width()),
                rect.top + rect.height()
            );
            drawText(
                lbl,
                DT_SINGLE | DT_ELLIPSIS,
                textRect,
                getMetric(METRIC_DEF_TEXT_SIZE).getUint8(),
                getColor(COLOR_WINDOW_TEXT),
                getDefaultFont()
            );
        }

    private:
        GfxContextPtr _gfxContext;
        const Font* _defaultFont = nullptr;
    };

    struct PackagedMessage
    {
        Message msg   = MSG_NONE;
        MsgParam p1   = 0;
        MsgParam p2   = 0;
    };

    using PackagedMessageQueue = std::queue<PackagedMessage>;

    class IWindow;
    class IWindowContainer
    {
    public:
        virtual bool hasChildren() = 0;
        virtual size_t childCount() = 0;
        virtual std::shared_ptr<IWindow> getChildByID(WindowID) = 0;
        virtual bool setForegroundWindow(const std::shared_ptr<IWindow>&) = 0;
        virtual void recalculateZOrder() = 0;
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

        virtual Rect getRect() const noexcept = 0;
        virtual void setRect(const Rect&) noexcept = 0;

        virtual Style getStyle() const noexcept = 0;
        virtual void setStyle(Style) noexcept = 0;

        virtual WindowID getID() const noexcept = 0;

        virtual uint8_t getZOrder() const noexcept = 0;
        virtual void setZOrder(uint8_t) noexcept = 0;

        virtual State getState() const noexcept = 0;
        virtual void setState(State) noexcept = 0;

        virtual std::string getText() const = 0;
        virtual void setText(const std::string&) = 0;

        virtual Color getBgColor() const noexcept = 0;
        virtual void setBgColor(Color) noexcept = 0;
        virtual Color getTextColor() const noexcept = 0;
        virtual void setTextColor(Color) noexcept = 0;
        virtual Color getFrameColor() const noexcept = 0;
        virtual void setFrameColor(Color) noexcept = 0;
        virtual Color getShadowColor() const noexcept = 0;
        virtual void setShadowColor(Color) noexcept = 0;

        virtual Coord getCornerRadius() const noexcept = 0;
        virtual void setCornerRadius(Coord) noexcept = 0;

        virtual bool routeMessage(Message, MsgParam, MsgParam) = 0;
        virtual bool queueMessage(Message, MsgParam, MsgParam) = 0;
        virtual bool processQueue(bool&) = 0;
        virtual bool processInput(InputParams*) = 0;

        virtual bool redraw(Rect* = nullptr, bool = false) = 0;
        virtual bool redrawChildren(Rect* = nullptr, bool = false) = 0;
        virtual bool hide() noexcept = 0;
        virtual bool show() noexcept = 0;
        virtual bool isVisible() const noexcept = 0;
        virtual bool isAlive() const noexcept = 0;
        virtual bool isDirty() const noexcept = 0;
        virtual void setDirty(bool) noexcept = 0;
        virtual bool isDrawable() const noexcept = 0;

        virtual bool destroy() = 0;

# if defined(TWM_DIAGNOSTICS)
        std::string typeName;
# endif

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
            for (const auto& win : _children) {
                if (id == win->getID()) {
                    return win;
                }
            }
            return nullptr;
        }

        bool setForegroundWindow(const WindowPtr& win) override
        {
# if !defined(TWM_SINGLETHREAD)
            ScopeLock lock(_childMtx);
# endif
            bool success = false;
            if (!win->getParent() && bitsHigh(win->getStyle(), STY_TOPLEVEL)) {
                for (auto it = _children.begin(); it != _children.end(); it++) {
                    if ((*it)->getID() == win->getID()) {
                        _children.erase(it);
                        _children.push_back(win);
                        success = true;
                        break;
                    }
                }
                if (success) {
                    recalculateZOrder();
                }
            }
            return success;
        }

        void recalculateZOrder() override
        {
# if !defined(TWM_SINGLETHREAD)
            ScopeLock lock(_childMtx);
# endif
            uint8_t zOrder = 0;
            for (const auto& win : _children) {
                win->setZOrder(zOrder++);
            }
        }

        bool addChild(const WindowPtr& child) override
        {
# if !defined(TWM_SINGLETHREAD)
            ScopeLock lock(_childMtx);
# endif
            if (getChildByID(child->getID()) != nullptr) {
                return false;
            }
            uint8_t zOrder = 0;
            auto last = _children.rbegin();
            if (last != _children.rend()) {
                zOrder = (*last)->getZOrder() + 1;
            }
            child->setZOrder(zOrder);
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
                    recalculateZOrder();
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
        struct Config
        {
            uint32_t minRenderIntervalMsec  = 0U;
            uint32_t minHitTestIntervalMsec = 0U;
        };

        static constexpr uint32_t DefaultMinRenderIntervalMsec = 100U;
        static constexpr uint32_t DefaultMinHitTestIntervalMsec = 200U;

        WindowManager() = delete;

        explicit WindowManager(
            const GfxDisplayPtr& gfxDisplay,
            const GfxContextPtr& gfxContext,
            const ThemePtr& theme,
            const Font* defaultFont,
            const Config* config = nullptr
        ) : _registry(std::make_shared<WindowContainer>()),
            _gfxDisplay(gfxDisplay), _gfxContext(gfxContext), _theme(theme)
        {
            TWM_ASSERT(_registry);
            TWM_ASSERT(_gfxDisplay);
            TWM_ASSERT(_gfxContext);
            TWM_ASSERT(_theme);
            _displayWidth  = _gfxContext->width();
            _displayHeight = _gfxContext->height();
            _theme->setGfxContext(_gfxContext);
            _theme->setDefaultFont(defaultFont);
            if (config) {
                _config = *config;
            } else {
                _config.minRenderIntervalMsec  = DefaultMinRenderIntervalMsec;
                _config.minHitTestIntervalMsec = DefaultMinHitTestIntervalMsec;
            }
        }

        virtual ~WindowManager()
        {
            tearDown();
        }

        void setState(State state) noexcept { _state = state; }
        State getState() const noexcept { return _state; }

        Config getConfig() const noexcept { return _config; }
        void setConfig(const Config& config) noexcept { _config = config; }

        void enableScreensaver(uint32_t activateAfterMsec) noexcept
        {
            _ssaverActivateAfter = activateAfterMsec;
            _ssaverEpoch = millis();
            setState(getState() | WMS_SSAVER_ENABLED);
            TWM_LOG(TWM_DEBUG, "screensaver enabled (%ums)", activateAfterMsec);
        }

        void disableScreensaver() noexcept
        {
            TWM_CONST(State, flags,
                WMS_SSAVER_ENABLED | WMS_SSAVER_ACTIVE | WMS_SSAVER_DRAWN);
            setState(getState() & ~flags);
            TWM_LOG(TWM_DEBUG, "screensaver disabled");
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

        Extent getDisplayWidth() const noexcept { return _displayWidth; }
        Extent getDisplayHeight() const noexcept { return _displayHeight; }

        Rect getDisplayRect() const noexcept
        {
            return Rect(0, 0, _displayWidth, _displayHeight);
        }

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
            if (id == WID_INVALID) {
                TWM_LOG(TWM_ERROR, "%hhu is a reserved window ID", WID_INVALID);
                return nullptr;
            }
            if (bitsHigh(style, STY_FULLSCREEN)) {
                x = 0;
                y = 0;
                width = getDisplayWidth();
                height = getDisplayHeight();
            }
            Rect rect(x, y, x + width, y + height);
            std::shared_ptr<TWindow> win(
                std::make_shared<TWindow>(
                    shared_from_this(), parent, id, style, rect, text
                )
            );
# if defined(TWM_DIAGNOSTICS)
            char* demangleBuf = reinterpret_cast<char*>(malloc(64));
            size_t dbufSize = 64;
            int status = 0;
            const auto dName = __cxxabiv1::__cxa_demangle(typeid(TWindow).name(),
                demangleBuf, &dbufSize, &status);
            TWM_ASSERT(status == 0);
            if (status == 0) {
                win->typeName = dName;
            }
            free(demangleBuf);
# endif
            if (bitsHigh(style, STY_CHILD) && !parent) {
                TWM_LOG(TWM_ERROR, "STY_CHILD && null parent");
                return nullptr;
            }
            if (bitsHigh(style, STY_TOPLEVEL) && parent) {
                TWM_LOG(TWM_ERROR, "STY_TOPLEVEL && parent");
                return nullptr;
            }
            if (preCreateHook && !preCreateHook(win)) {
                TWM_LOG(TWM_ERROR, "pre-create hook failed");
                return nullptr;
            }
            if (!win->routeMessage(MSG_CREATE)) {
                TWM_LOG(TWM_ERROR, "MSG_CREATE = false");
                return nullptr;
            }
            bool dupe = parent ? !parent->addChild(win) : !_registry->addChild(win);
            if (dupe) {
                TWM_LOG(TWM_ERROR, "duplicate window ID %hhu (parent: %hhu)",
                    id, parent ? parent->getID() : WID_INVALID);
                return nullptr;
            }
            if (bitsHigh(win->getStyle(), STY_AUTOSIZE)) {
                win->routeMessage(MSG_RESIZE);
            }
            win->redraw();
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
            const Extent width = min(
                _theme->getMetric(METRIC_MAX_PROMPT_CX).getExtent(),
                static_cast<Extent>(
                    getDisplayWidth() - (_theme->getMetric(METRIC_X_PADDING).getExtent() * 2)
                )
            );
            const Extent height = min(
                _theme->getMetric(METRIC_MAX_PROMPT_CY).getExtent(),
                static_cast<Extent>(
                    getDisplayHeight() - (_theme->getMetric(METRIC_Y_PADDING).getExtent() * 2)
                )
            );
            auto prompt = createWindow<TPrompt>(
                parent,
                id,
                style,
                (getDisplayWidth() / 2) - (width / 2),
                (getDisplayHeight() / 2) - (height / 2),
                width,
                height,
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
            auto pbar = createWindow<TBar>(
                parent, id, style, x, y, width, height
            );
            if (pbar) {
                pbar->setProgressBarStyle(pbarStyle);
            }
            return pbar;
        }

        bool setForegroundWindow(const WindowPtr& win)
        {
            return _registry->setForegroundWindow(win);
        }

        void hitTest(Coord x, Coord y)
        {
            if (millis() - _lastHitTestTime < _config.minHitTestIntervalMsec) {
                return;
            }
            TWM_ASSERT(x > 0 && y > 0);
            TWM_ASSERT(x <= _displayWidth && y <= _displayHeight);
            TWM_LOG(TWM_DEBUG, "hit test @ %hd, %hd", x, y);
            if (bitsHigh(getState(), WMS_SSAVER_ENABLED)) {
                _ssaverEpoch = millis();
                if (bitsHigh(getState(), WMS_SSAVER_ACTIVE)) {
                    return;
                }
            }
            bool claimed = false;
            _registry->forEachChildReverse([&](const WindowPtr& child)
            {
                if (!child->isDrawable()) {
                    return true;
                }
# if defined(TWM_DIAGNOSTICS)
                TWM_LOG(TWM_DEBUG, "interrogating %s (%hhu) about hit test @ %hd, %hd",
                    child->typeName.c_str(), child->getID(), x, y);
# endif
                InputParams params;
                params.type = INPUT_TAP;
                params.x    = x;
                params.y    = y;
                if (child->processInput(&params)) {
# if defined(TWM_DIAGNOSTICS)
                    TWM_LOG(TWM_DEBUG, "%s (%hhu) claimed hit test @ %hd, %hd",
                        child->typeName.c_str(), params.handledBy, x, y);
# endif
                    claimed = true;
                    return false;
                }
                return true;
            });
            if (!claimed) {
                TWM_LOG(TWM_DEBUG, "hit test @ %hd, %hd unclaimed", x, y);
            }
            _lastHitTestTime = millis();
        }

        bool isWindowEntirelyCovered(const WindowPtr& win)
        {
            bool covered = false;
            const auto rect = win->getRect();
            _registry->forEachChildReverse([&](const WindowPtr& other)
            {
                if (other == win) {
                    return false;
                }
                if (!other->isDrawable()) {
                    return true;
                }
                if (rect.withinRect(other->getRect())) {
                    covered = true;
                    return false;
                }
                return true;
            });
            return covered;
        }

        virtual void setDirtyRect(const Rect& rect)
        {
            _registry->forEachChild([=](const WindowPtr& win)
            {
                if (!win->isDrawable() || win->isDirty()) {
                    return true;
                }
                if (win->getRect().intersectsRect(rect)) {
                    win->setDirty(true);
                }
                return true;
            });
        }

        virtual void update()
        {
            if (millis() - _lastRenderTime < _config.minRenderIntervalMsec) {
                return;
            }
#if defined(TWM_DIAGNOSTICS)
            const auto beginTime = micros();
#endif
            bool updated = false;
            if (bitsHigh(getState(), WMS_SSAVER_ENABLED)) {
                if (millis() - _ssaverEpoch >= _ssaverActivateAfter) {
                    if (!bitsHigh(getState(), WMS_SSAVER_ACTIVE)) {
                        setState(getState() | WMS_SSAVER_ACTIVE);
                        TWM_LOG(TWM_DEBUG, "activated screensaver");
                    }
                } else {
                    if (bitsHigh(getState(), WMS_SSAVER_ACTIVE)) {
                        setState(getState() & ~(WMS_SSAVER_ACTIVE | WMS_SSAVER_DRAWN));
                        setDirtyRect(getDisplayRect());
                        TWM_LOG(TWM_DEBUG, "de-activated screensaver");
                    }
                }
            }
            if (bitsHigh(getState(), WMS_SSAVER_ACTIVE)) {
                if (!bitsHigh(getState(), WMS_SSAVER_DRAWN)) {
                    _theme->drawScreensaver();
                    updated = true;
                    setState(getState() | WMS_SSAVER_DRAWN);
                }
            } else {
                uint8_t zOrderHighWaterMark = 0;
                Rect dirtyRect(0, 0, 0, 0);

                auto maybeDrawWindow = [&](const WindowPtr& win, const Rect& rect,
                    uint8_t zOrder, bool draw, bool force, bool pseudo)
                {
                    bool drawn = false;
                    Rect drawnRect;
                    if (draw && !pseudo) {
                        drawn = win->redraw(&drawnRect, force);
                        if (drawn) {
                            dirtyRect.mergeRect(drawnRect);
                        }
                    }
                    if (drawn || pseudo) {
                        if (zOrder > zOrderHighWaterMark) {
                            zOrderHighWaterMark = zOrder;
                        }
                        dirtyRect.mergeRect(pseudo ? rect : drawnRect);
                    }
                    return drawn || pseudo;
                };

                _registry->forEachChild([&](const WindowPtr& win)
                {
                    bool redrawnProcessing = false;
                    while (
                        win->processQueue(redrawnProcessing)
                    );
                    if (redrawnProcessing) {
                        if (maybeDrawWindow(win, win->getRect(), win->getZOrder(),
                            false, false, true)) {
                            updated = true;
                        }
                    }

                    const auto rect   = win->getRect();
                    const auto zOrder = win->getZOrder();

                    bool lowerDrawn  = updated && zOrderHighWaterMark < zOrder;
                    bool dirty       = win->isDirty();
                    bool intersectsDirtyRect = rect.intersectsRect(dirtyRect);
                    bool invalidated = dirty || (lowerDrawn && intersectsDirtyRect);
                    bool drawable    = win->isDrawable();
                    bool pseudoDraw  = drawable && isWindowEntirelyCovered(win);

                    bool drawn = maybeDrawWindow(
                        win,
                        rect,
                        zOrder,
                        (invalidated || dirty) && drawable,
                        invalidated && !dirty && !pseudoDraw,
                        pseudoDraw && !(invalidated && drawable)
                    );
                    if (drawn) {
                        updated = true;
                    } else {
                        if (!pseudoDraw) {
                            Rect drawnRect;
                            if (win->redrawChildren(&drawnRect)) {
                                updated = true;
                                dirtyRect.mergeRect(drawnRect);
                            }
                        }
                    }
                    return true;
                });
            }
#if defined(TWM_DIAGNOSTICS)
            _renderAccumTime += micros() - beginTime;
            _renderAccumCount++;
#endif
            if (updated) {
                render();
            }
            _lastRenderTime = millis();
        }

        template<typename... TDisplayArgs>
        inline bool begin(TDisplayArgs&&... args)
        {
# if !defined(ADAFRUIT_RA8875)
            TWM_ASSERT(_gfxDisplay);
            TWM_ASSERT(_gfxContext);
            bool success = _gfxDisplay && _gfxContext;
# else
            TWM_ASSERT(_gfxContext);
            bool success = _gfxContext;
# endif
            if (success) {
#if defined(ESP32) && defined(BOARD_HAS_PSRAM) && !defined(ADAFRUIT_RA8875)
                bool alloc_failed = _gfxContext->getBuffer() == nullptr;
                if (alloc_failed) {
                    auto buf_size = _gfxContext->width() * _gfxContext->height()
                        * sizeof(uint16_t);
                    TWM_LOG(TWM_WARN, "malloc of %u bytes failed! err: %d", buf_size, errno);
                    auto new_buffer = ps_calloc(1, buf_size);
                    success = new_buffer != nullptr;
                    if (!success) {
                        TWM_LOG(TWM_ERROR, "ps_calloc of %u bytes failed! err: %d", buf_size, errno);
                        return false;
                    }
                    _gfxContext->setBuffer(static_cast<uint16_t*>(new_buffer));
                }
#endif
# if defined(TWM_GFX_ADAFRUIT)
                /// TODO: possible in C++11 to deduce return type
                /// of begin() and capture it if it's bool?
#  if defined(ADAFRUIT_RA8875)
                _gfxContext->begin(args...);
#  else
                _gfxDisplay->begin(args...);
#  endif
# elif defined(TWM_GFX_ARDUINO)
                success &= _gfxDisplay->begin(args...);
                success &= _gfxContext->begin(GFX_SKIP_OUTPUT_BEGIN);
# endif
            }
            TWM_ASSERT(success);
            if (success) {
                _displayWidth = _gfxContext->width();
                _displayHeight = _gfxContext->height();
                TWM_LOG(TWM_DEBUG, "display size %hux%hu", _displayWidth, _displayHeight);
            }
            return success;
        }

        void render()
        {
#if defined(TWM_DIAGNOSTICS)
            static constexpr uint32_t reportInterval = 10000U;
            static uint32_t lastReport = 0;
            const auto beginTime = micros();
#endif
# if defined(TWM_GFX_ADAFRUIT)
#  if !defined(ADAFRUIT_RA8875)
            _gfxDisplay->drawRGBBitmap(0, 0, _gfxContext->getBuffer(),
                getDisplayWidth(), getDisplayHeight());
#  else
            //_gfxDisplay->flush();
#  endif
# elif defined(TWM_GFX_ARDUINO)
            _gfxContext->flush();
# endif
#if defined(TWM_DIAGNOSTICS)
            if (millis() - lastReport > reportInterval) {
                _copyFrameAccumCount = max(1U, _copyFrameAccumCount);
                _renderAccumCount    = max(1U, _renderAccumCount);
                _copyFrameAvg        = _copyFrameAccumTime / _copyFrameAccumCount;
                _renderAvg           = _renderAccumTime / _renderAccumCount;
                _renderAccumTime     = 0U;
                _renderAccumCount    = 0U;
                _copyFrameAccumTime  = 0U;
                _copyFrameAccumCount = 0U;
                TWM_LOG(TWM_DEBUG, "avg. render: %uμs, copy: %uμs, total: %uμs",
                    _renderAvg, _copyFrameAvg, (_renderAvg + _copyFrameAvg));
                lastReport = millis();
                return;
            }
            _copyFrameAccumTime += micros() - beginTime;
            _copyFrameAccumCount++;
#endif
        }

    protected:
        Config _config;
        WindowContainerPtr _registry;
        GfxDisplayPtr _gfxDisplay;
        GfxContextPtr _gfxContext;
        ThemePtr _theme;
        State _state                  = 0;
        Extent _displayWidth          = 0;
        Extent _displayHeight         = 0;
        uint32_t _ssaverEpoch         = 0U;
        uint32_t _ssaverActivateAfter = 0U;
        uint32_t _lastRenderTime      = 0U;
        uint32_t _lastHitTestTime     = 0U;
#if defined(TWM_DIAGNOSTICS)
        uint32_t _renderAvg           = 0U;
        uint32_t _copyFrameAvg        = 0U;
        uint32_t _renderAccumTime     = 0U;
        uint32_t _renderAccumCount    = 0U;
        uint32_t _copyFrameAccumTime  = 0U;
        uint32_t _copyFrameAccumCount = 0U;
#endif
    };

    using WindowManagerPtr = std::shared_ptr<WindowManager>;

    template<class TTheme, class TGfxDisplay>
    WindowManagerPtr createWindowManager(
        const std::shared_ptr<TGfxDisplay>& display,
        const std::shared_ptr<GfxContext>& context,
        const std::shared_ptr<TTheme>& theme,
        const Font* defaultFont
    )
    {
        static_assert(std::is_base_of<GfxDisplay, TGfxDisplay>::value);
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
        ) : _wm(wm), _parent(parent), _rect(rect), _text(text), _style(style), _id(id)
        {
            auto theme = _getTheme();
            TWM_ASSERT(theme != nullptr);
            _bgColor     = theme->getColor(COLOR_WINDOW_BG);
            _textColor   = theme->getColor(COLOR_WINDOW_TEXT);
            _frameColor  = theme->getColor(COLOR_WINDOW_FRAME);
            _shadowColor = theme->getColor(COLOR_WINDOW_SHADOW);
        }

        virtual ~Window() = default;

        bool hasChildren() override { return _children.hasChildren(); }
        size_t childCount() override { return _children.childCount(); }
        WindowPtr getChildByID(WindowID id) override { return _children.getChildByID(id); }

        bool setForegroundWindow(const WindowPtr& win) override
        {
            TWM_UNUSED(win);
            return false;
        }

        void recalculateZOrder() override { _children.recalculateZOrder(); }
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

        Rect getRect() const noexcept override { return _rect; }

        void setRect(const Rect& rect) noexcept override
        {
            if (rect != _rect) {
                _rect = rect;
                setDirty(true);
            }
        }

        Style getStyle() const noexcept override { return _style; }

        void setStyle(Style style) noexcept override
        {
            if (style != _style) {
                _style = style;
                setDirty(true);
            }
        }

        WindowID getID() const noexcept override { return _id; }

        uint8_t getZOrder() const noexcept override { return _zOrder; }
        void setZOrder(uint8_t zOrder) noexcept override { _zOrder = zOrder; }

        State getState() const noexcept override { return _state; }
        void setState(State state) noexcept override { _state = state; }

        std::string getText() const override { return _text; }

        void setText(const std::string& text) override
        {
            if (text != _text) {
                _text = text;
                setDirty(true);
            }
        }

        Color getBgColor() const noexcept override { return _bgColor; }

        void setBgColor(Color color) noexcept override
        {
            if (color != _bgColor) {
                _bgColor = color;
                setDirty(true);
            }
        }

        Color getTextColor() const noexcept override { return _textColor; }

        void setTextColor(Color color) noexcept override
        {
            if (color != _textColor) {
                _textColor = color;
                setDirty(true);
            }
        }

        Color getFrameColor() const noexcept override { return _frameColor; }

        void setFrameColor(Color color) noexcept override
        {
            if (color != _frameColor) {
                _frameColor = color;
                setDirty(true);
            }
        }

        Color getShadowColor() const noexcept override { return _shadowColor; }

        void setShadowColor(Color color) noexcept override
        {
            if (color != _shadowColor) {
                _shadowColor = color;
                setDirty(true);
            }
        }

        Coord getCornerRadius() const noexcept override { return _cornerRadius; }

        void setCornerRadius(Coord radius) noexcept override
        {
            if (radius != _cornerRadius) {
                _cornerRadius = radius;
                setDirty(true);
            }
        }

        bool routeMessage(Message msg, MsgParam p1 = 0, MsgParam p2 = 0) override
        {
            bool handled = false;
            bool dirty   = false;
            switch (msg) {
                case MSG_CREATE: {
                    dirty = handled = onCreate(p1, p2);
                    if (handled) {
                        setState(getState() | STA_ALIVE);
                    }
                    break;
                }
                case MSG_DESTROY: {
                    handled = onDestroy(p1, p2);
                    setState(getState() & ~STA_ALIVE);
                    break;
                }
                case MSG_DRAW: {
                    if (!isDrawable()) {
                        break;
                    }
                    if (!isDirty() && p1 == 0U) {
                        break;
                    }
                    handled = onDraw(p1, p2);
                    setDirty(false);
                    break;
                }
                case MSG_INPUT: {
                    dirty = handled = onInput(p1, p2);
                    break;
                }
                case MSG_EVENT: return onEvent(p1, p2);
                case MSG_RESIZE: {
                    dirty = handled = onResize(p1, p2);
                    break;
                }
                default:
                    TWM_ASSERT(false);
                    return false;
            }
            if (dirty) {
                setDirty(true);
            }
            return handled;
        }

        bool queueMessage(Message msg, MsgParam p1 = 0, MsgParam p2 = 0) override
        {
# if !defined(TWM_SINGLETHREAD)
            ScopeLock lock(_queueMtx);
# endif
            PackagedMessage pm;
            pm.msg    = msg;
            pm.p1     = p1;
            pm.p2     = p2;
            _queue.push(pm);
            return msg == MSG_INPUT && getMsgParamLoWord(p1) == INPUT_TAP;
        }

        bool processQueue(bool& redrawn) override
        {
# if !defined(TWM_SINGLETHREAD)
            ScopeLock lock(_queueMtx);
# endif
            if (!_queue.empty()) {
                auto pm = _queue.front();
                _queue.pop();
                if (routeMessage(pm.msg, pm.p1, pm.p2) && pm.msg == MSG_DRAW) {
                    redrawn = true;
                }
            }
            forEachChild([&](const WindowPtr& child)
            {
                child->processQueue(redrawn);
                return true;
            });
            return !_queue.empty();
        }

        bool processInput(InputParams* params) override
        {
            if (!isDrawable()) {
                return false;
            }
            if (!getRect().pointWithin(params->x, params->y)) {
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

        bool redraw(Rect* dirtyRect = nullptr, bool force = false) override
        {
            if (!isDrawable()) {
                return false;
            }
            bool redrawn = (isDirty() || force)
                ? routeMessage(MSG_DRAW, force ? 1U : 0U) : false;
            bool childRedrawn = false;
            if (redrawn) {
                if (dirtyRect != nullptr) {
                    dirtyRect->mergeRect(getRect());
                }
                forEachChild([](const WindowPtr& child)
                {
                    child->setDirty(true);
                    return true;
                });
            }
            childRedrawn = redrawChildren(dirtyRect, force);
            return redrawn || childRedrawn;
        }

        bool redrawChildren(Rect* dirtyRect = nullptr, bool force = false) override
        {
            bool childRedrawn = false;
            forEachChild([&](const WindowPtr& child)
            {
                if (child->redraw(dirtyRect, force)) {
                    childRedrawn = true;
                }
                return true;
            });
            return childRedrawn;
        }

        bool hide() noexcept override
        {
            if (!isVisible()) {
                return false;
            }
            setStyle(getStyle() & ~STY_VISIBLE);
            auto wm = _getWM();
            TWM_ASSERT(wm);
            wm->setDirtyRect(getRect());
            return true;
        }

        bool show() noexcept override
        {
            const auto topLevel = bitsHigh(getStyle(), STY_TOPLEVEL);
            TWM_ASSERT(!topLevel || !getParent());
            if (!topLevel && isVisible()) {
                return false;
            }
            auto shown = true;
            if (topLevel) {
                auto wm = _getWM();
                shown = wm->setForegroundWindow(shared_from_this());
            }
            setStyle(getStyle() | STY_VISIBLE);
            setDirty(true);
            return shown;
        }

        bool isVisible() const noexcept override
        {
            const auto rect = getRect();
            return bitsHigh(getStyle(), STY_VISIBLE) && rect.width() > 0 &&
                rect.height() > 0;
        }

        bool isAlive() const noexcept override
        {
            return bitsHigh(getState(), STA_ALIVE);
        }

        bool isDirty() const noexcept override
        {
            return bitsHigh(getState(), STA_DIRTY);
        }

        void setDirty(bool dirty) noexcept override
        {
            if (dirty) {
                setState(getState() | STA_DIRTY);
                forEachChild([&](const WindowPtr& child)
                {
                    child->setDirty(true);
                    return true;
                });
            } else {
                setState(getState() & ~STA_DIRTY);
            }
        }

        bool isDrawable() const noexcept override
        {
            const auto wm = _getWM();
            const auto parent = getParent();
            return isVisible() &&
                   isAlive() &&
                   (!parent || (parent && parent->isDrawable())) &&
                   !getRect().outsideRect(wm->getDisplayRect());
        }

        bool destroy() override
        {
            hide();
            bool destroyed = routeMessage(MSG_DESTROY);
            forEachChild([&](const WindowPtr& child)
            {
                destroyed &= child->destroy();
                return true;
            });
            removeAllChildren();
            return destroyed;
        }

    protected:
        // ====== Begin message handlers ======

        // MSG_CREATE: p1 = 0, p2 = 0.
        bool onCreate(MsgParam p1, MsgParam p2) override
        {
            auto theme = _getTheme();
            TWM_ASSERT(theme != nullptr);
            setCornerRadius(theme->getMetric(METRIC_CORNER_RADIUS_WINDOW).getCoord());
            return true;
        }

        // MSG_DESTROY: p1 = 0, p2 = 0.
        bool onDestroy(MsgParam p1, MsgParam p2) override
        {
            TWM_UNUSED(p1);
            TWM_UNUSED(p2);
            return true;
        }

        // MSG_DRAW: p1 = 1 (force) || 0, p2 = 0.
        bool onDraw(MsgParam p1, MsgParam p2) override
        {
            auto theme = _getTheme();
            TWM_ASSERT(theme != nullptr);
            theme->drawWindowBackground(getRect(), getCornerRadius(), getBgColor());
            if (bitsHigh(getStyle(), STY_FRAME)) {
                theme->drawWindowFrame(getRect(), getCornerRadius(), getFrameColor());
            }
            if (bitsHigh(getStyle(), STY_SHADOW)) {
                theme->drawWindowShadow(getRect(), getCornerRadius(), getShadowColor());
            }
            return true;
        }

        // MSG_INPUT: p1 = (loword: type), p2 = (hiword: x, loword: y).
        // Returns true if the input event was consumed by this window, false otherwise.
        bool onInput(MsgParam p1, MsgParam p2) override
        {
            InputParams params;
            params.type = static_cast<InputType>(getMsgParamLoWord(p1));
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

        // MSG_EVENT: p1 = EventType, p2 = child WindowID.
        bool onEvent(MsgParam p1, MsgParam p2) override { return true; }

        // MSG_RESIZE: p1 = 0, p2 = 0.
        bool onResize(MsgParam p1, MsgParam p2) override
        {
            TWM_ASSERT(bitsHigh(getStyle(), STY_AUTOSIZE));
            return false;
        }

        // ====== End message handlers ======

        bool onTapped(Coord x, Coord y) override {
            TWM_UNUSED(x);
            TWM_UNUSED(y);
            return false;
        }

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
        std::string _text;
        Style _style        = 0;
        WindowID _id        = WID_INVALID;
        uint8_t _zOrder     = 0;
        State _state        = 0;
        Color _bgColor      = 0;
        Color _textColor    = 0;
        Color _frameColor   = 0;
        Color _shadowColor  = 0;
        Coord _cornerRadius = 0;
    };

    class Button : public Window
    {
    public:
        using Window::Window;
        virtual ~Button() = default;

        bool onTapped(Coord x, Coord y) override
        {
            _lastTapped = millis();
            auto parent = getParent();
            TWM_ASSERT(parent != nullptr);
            if (parent) {
                parent->queueMessage(MSG_EVENT, EVT_CHILD_TAPPED, getID());
            }
            return parent != nullptr;
        }

        bool onCreate(MsgParam p1, MsgParam p2) override
        {
            if (!Window::onCreate(p1, p2)) {
                return false;
            }

            auto theme = _getTheme();
            TWM_ASSERT(theme != nullptr);
            setCornerRadius(theme->getMetric(METRIC_CORNER_RADIUS_BUTTON).getCoord());
            return true;
        }

        bool onDraw(MsgParam p1, MsgParam p2) override
        {
            auto theme = _getTheme();
            TWM_ASSERT(theme != nullptr);
            const bool pressed = (millis() - _lastTapped <
                theme->getMetric(METRIC_BUTTON_TAPPED_DURATION).getUint32());
            theme->drawWindowBackground(
                getRect(),
                theme->getMetric(METRIC_CORNER_RADIUS_BUTTON).getCoord(),
                theme->getColor(pressed ? COLOR_BUTTON_BG_PRESSED : COLOR_BUTTON_BG)
            );
            theme->drawWindowFrame(
                getRect(),
                theme->getMetric(METRIC_CORNER_RADIUS_BUTTON).getCoord(),
                theme->getColor(pressed ? COLOR_BUTTON_FRAME_PRESSED : COLOR_BUTTON_FRAME)
            );
            theme->drawText(
                getText().c_str(),
                DT_SINGLE | DT_CENTER,
                getRect(),
                theme->getMetric(METRIC_DEF_TEXT_SIZE).getUint8(),
                theme->getColor(pressed ? COLOR_BUTTON_TEXT_PRESSED : COLOR_BUTTON_TEXT),
                theme->getDefaultFont()
            );
            return true;
        }

        bool onResize(MsgParam p1, MsgParam p2) override
        {
            auto gfx = _getGfxContext();
            TWM_ASSERT(gfx != nullptr);
            auto theme = _getTheme();
            TWM_ASSERT(theme != nullptr);
            Coord x, y;
            Extent width, height;
            auto rect = getRect();
            gfx->getTextBounds(getText().c_str(), rect.left, rect.top, &x, &y, &width, &height);
            const auto maxWidth = max(width, theme->getMetric(METRIC_DEF_BUTTON_CX).getExtent());
            rect.right = rect.left + maxWidth +
                (theme->getMetric(METRIC_BUTTON_LABEL_PADDING).getExtent() * 2);
            rect.bottom = rect.top + theme->getMetric(METRIC_DEF_BUTTON_CY).getExtent();
            setRect(rect);
            /// TODO: if not autosize, clip label, perhaps with ellipsis.
            return true;
        }

    protected:
        u_long _lastTapped = 0UL;
    };

    class Label : public Window
    {
    public:
        using Window::Window;
        Label() = default;
        virtual ~Label() = default;

        bool onDraw(MsgParam p1, MsgParam p2) override
        {
            auto theme = _getTheme();
            TWM_ASSERT(theme != nullptr);
            theme->drawWindowBackground(getRect(), getCornerRadius(), getBgColor());
            theme->drawText(
                getText().c_str(),
                DT_SINGLE | DT_ELLIPSIS,
                getRect(),
                theme->getMetric(METRIC_DEF_TEXT_SIZE).getUint8(),
                getTextColor(),
                theme->getDefaultFont()
            );
            return true;
        }
    };

    class MultilineLabel : public Window
    {
    public:
        using Window::Window;
        MultilineLabel() = default;
        virtual ~MultilineLabel() = default;

        bool onDraw(MsgParam p1, MsgParam p2) override
        {
            auto theme = _getTheme();
            TWM_ASSERT(theme != nullptr);
            theme->drawWindowBackground(getRect(), getCornerRadius(), getBgColor());
            theme->drawText(
                getText().c_str(),
                DT_CENTER,
                getRect(),
                theme->getMetric(METRIC_DEF_TEXT_SIZE).getUint8(),
                getTextColor(),
                theme->getDefaultFont()
            );
            return true;
        }
    };

    class Prompt : public Window
    {
    public:
        TWM_CONST(WindowID, LabelID, 1);

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
            TWM_ASSERT(wm != nullptr);
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

        void setText(const std::string& text) override
        {
            if (_label) {
                _label->setText(text);
            }
        }

        bool onCreate(MsgParam p1, MsgParam p2) override
        {
            auto wm = _getWM();
            TWM_ASSERT(wm != nullptr);
            auto theme = _getTheme();
            TWM_ASSERT(theme != nullptr);
            setCornerRadius(theme->getMetric(METRIC_CORNER_RADIUS_PROMPT).getCoord());
            setBgColor(theme->getColor(COLOR_PROMPT_BG));
            setFrameColor(theme->getColor(COLOR_PROMPT_FRAME));
            setShadowColor(theme->getColor(COLOR_PROMPT_SHADOW));
            const auto rect = getRect();
            const auto xPadding = theme->getMetric(METRIC_X_PADDING).getExtent();
            const auto yPadding = theme->getMetric(METRIC_Y_PADDING).getExtent();
            const auto defBtnHeight = theme->getMetric(METRIC_DEF_BUTTON_CY).getExtent();
            _label = wm->createWindow<MultilineLabel>(
                shared_from_this(),
                LabelID,
                STY_CHILD | STY_VISIBLE | STY_LABEL,
                rect.left + xPadding,
                rect.top + yPadding,
                rect.width() - (xPadding * 2),
                rect.height() - ((yPadding * 3) + defBtnHeight),
                getText()
            );
            if (!_label) {
                return false;
            }
            _label->setBgColor(theme->getColor(COLOR_PROMPT_BG));
            auto rectLbl = _label->getRect();
            /// TODO: refactor this. prompts should have styles, such as
            // PROMPT_1BUTTON and PROMPT_2BUTTON. they should then take
            // the appropriate button metadata in their constructor
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
                auto rectBtn = child->getRect();
                rectBtn.top = rectLbl.bottom + yPadding;
                rectBtn.bottom = rectBtn.top + defBtnHeight;
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
                            rectBtn.left = rect.left + xPadding;
                        }
                        break;
                        default:
                            TWM_ASSERT(false);
                        return false;
                    }
                    rectBtn.right = rectBtn.left + width;
                } else {
                    rectBtn.right = rect.right - xPadding;
                    rectBtn.left = rectBtn.right - width;
                }
                child->setRect(rectBtn);
                return true;
            });

            return true;
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

        void setProgressBarStyle(Style barStyle) noexcept
        {
            if (barStyle != _barStyle) {
                _barStyle = barStyle;
                setDirty(true);
            }
        }

        float getProgressValue() const noexcept { return _value; }

        void setProgressValue(float value) noexcept
        {
            if (abs(value) != abs(_value)) {
                _value = value;
                setDirty(true);
            }
        }

    protected:
        bool onDraw(MsgParam p1, MsgParam p2) override
        {
            auto theme = _getTheme();
            TWM_ASSERT(theme != nullptr);
            theme->drawProgressBarBackground(getRect());
            theme->drawWindowFrame(getRect(), getCornerRadius(), getFrameColor());
            if (bitsHigh(getProgressBarStyle(), PBR_NORMAL)) {
                theme->drawProgressBarProgress(getRect(), getProgressValue());
                return true;
            }
            if (bitsHigh(getProgressBarStyle(), PBR_INDETERMINATE)) {
                theme->drawProgressBarIndeterminate(getRect(), getProgressValue());
                return true;
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
                if (checked) {
                    setState(getState() | STA_CHECKED);
                } else {
                    setState(getState() & ~STA_CHECKED);
                }
                setDirty(true);
            }
        }

        bool isChecked() const noexcept { return bitsHigh(getState(), STA_CHECKED); }

    protected:
        bool onDraw(MsgParam p1, MsgParam p2) override
        {
            auto theme = _getTheme();
            TWM_ASSERT(theme != nullptr);
            theme->drawCheckBox(getText().c_str(), isChecked(), getRect());
            return true;
        }

        bool onTapped(Coord x, Coord y) override
        {
            auto theme = _getTheme();
            TWM_ASSERT(theme != nullptr);
            if (millis() - _lastToggle >=
                theme->getMetric(METRIC_CHECKBOX_CHECK_DELAY).getUint32()) {
                setChecked(!isChecked());
                _lastToggle = millis();
            }
            return true;
        }

    private:
        u_long _lastToggle = 0UL;
    };
} // namespace thumby

#endif // !_THUMBY_WM_H_INCLUDED
