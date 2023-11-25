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
# include <initializer_list>
# include <stdexcept>
# include <functional>
# include <string>
# include <memory>
# include <vector>
# include <queue>
# include <mutex>
# include <map>

# include <Adafruit_GFX.h>
# include <Fonts/FreeSans9pt7b.h>

# include <glcdfont.c>
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
            TWM_LOG(TWM_ERROR, "assert! '" #expr "' at %s:%d", __FILE__, __LINE__); \
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

    /** Predefined (reserved) window identifiers. */
    TWM_CONST(WindowID, WID_INVALID,  -1);
    TWM_CONST(WindowID, WID_DESKTOP,   1);
    TWM_CONST(WindowID, WID_PROMPT,    2);
    TWM_CONST(WindowID, WID_PROMPTLBL, 3);

    /** Window style. */
    using Style = uint16_t;

    /** Window state. */
    using State = uint16_t;

    /** Window message parameter type. */
    using MsgParam = uintptr_t;

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

        inline bool isPointWithin(const Point& point) const noexcept
        {
            if (point.x >= left && point.x <= left + width() &&
                point.y >= top  && point.y <= top + height()) {
                return true;
            }
            return false;
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
        STY_TA_LEFT   = 1 << 3,
        STY_TA_CNTR   = 1 << 4
    } _WindowStyleFlags;

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
        InputType type = INPUT_NONE;
        Coord x = 0;
        Coord y = 0;
    };

    class ITheme
    {
    public:
        virtual void setGfxDriver(const GfxDriverPtr& gfx) = 0;
        virtual void drawBlankScreen() const = 0;
        virtual Extent getWindowXPadding() const = 0;
        virtual Extent getWindowYPadding() const = 0;
        virtual uint8_t getButtonTextSize() const = 0;
        virtual Extent getButtonWidth() const = 0;
        virtual Extent getButtonHeight() const = 0;
        virtual Extent getButtonLabelPadding() const = 0;
        virtual void drawWindowFrame(const Rect& rect) const = 0;
        virtual void drawWindowBackground(const Rect& rect) const = 0;
        virtual void drawWindowText(const char* text, uint8_t flags,
            const Rect& rect) const = 0;
        virtual void drawButtonFrame(bool pressed, const Rect& rect) const = 0;
        virtual void drawButtonBackground(bool pressed, const Rect& rect) const = 0;
        virtual void drawButtonLabel(const char* lbl, bool pressed, const Rect& rect) const = 0;
    };

    using ThemePtr = std::shared_ptr<ITheme>;

    class DefaultTheme : public ITheme
    {
    public:
        TWM_CONST(Color, BlankScreenColor, 0x0000);
        TWM_CONST(Extent, WindowXPadding, 20);
        TWM_CONST(Extent, WindowYPadding, 20);
        TWM_CONST(Extent, WindowFrameThickness, 1);
        TWM_CONST(Color, WindowFrameColor, 0x7bef);
        TWM_CONST(Color, WindowFrameShadowColor, 0xad75);
        TWM_CONST(Color, WindowBgColor, 0xc618);
        TWM_CONST(Color, WindowTextColor, 0x0000);
        TWM_CONST(Extent, ButtonWidth, 90);
        TWM_CONST(Extent, ButtonHeight, 40);
        TWM_CONST(Coord, ButtonCornerRadius, 4);
        TWM_CONST(Color, ButtonFrameColor, 0x4208);
        TWM_CONST(Color, ButtonBgColor, 0x7bef);
        TWM_CONST(Color, ButtonLabelColor, 0xffff);
        TWM_CONST(Extent, ButtonLabelPadding, 10);
        TWM_CONST(Color, ButtonFrameColorPressed, 0x4208);
        TWM_CONST(Color, ButtonBgColorPressed, 0x4208);
        TWM_CONST(Color, ButtonLabelColorPressed, 0xffff);
        TWM_CONST(uint8_t, WindowTextSize, 1);
        TWM_CONST(uint8_t, ButtonTextSize, 1);
        TWM_CONST(Coord, WindowTextYOffset, 4);
        TWM_CONST(const GFXfont*, WindowTextFont, &FreeSans9pt7b);

        void setGfxDriver(const GfxDriverPtr& gfx)
        {
            TWM_ASSERT(gfx);
            _gfx = gfx;
        }

        void drawBlankScreen() const final
        {
            _gfx->fillScreen(BlankScreenColor);
        }

        Extent getWindowXPadding() const final { return WindowXPadding; }
        Extent getWindowYPadding() const final { return WindowYPadding; }
        uint8_t getButtonTextSize() const final { return ButtonTextSize; }
        Extent getButtonWidth() const final { return ButtonWidth; }
        Extent getButtonHeight() const final { return ButtonHeight; }
        Extent getButtonLabelPadding() const final { return ButtonLabelPadding; }

        void drawWindowFrame(const Rect& rect) const final
        {
            Rect tmp = rect;
            tmp.deflate(WindowFrameThickness);
            _gfx->drawRect(tmp.left, tmp.top, tmp.width(), tmp.height(),
                WindowFrameColor);
            _gfx->drawFastHLine(
                rect.left + (WindowFrameThickness * 2),
                rect.bottom - WindowFrameThickness,
                rect.width() - (WindowFrameThickness * 2),
                WindowFrameShadowColor
            );
            _gfx->drawFastVLine(
                rect.right - WindowFrameThickness,
                rect.top + (WindowFrameThickness * 2),
                rect.height() - (WindowFrameThickness * 2),
                WindowFrameShadowColor
            );
        }

        void drawWindowBackground(const Rect& rect) const final
        {
            _gfx->fillRect(rect.left, rect.top, rect.width(), rect.height(),
                WindowBgColor);
        }

        void drawWindowText(const char* text, uint8_t flags, const Rect& rect) const final
        {
            _gfx->setTextSize(WindowTextSize);
            _gfx->setTextColor(WindowTextColor);

            bool xCenter = bitsHigh(flags, DTF_CENTER);
            bool singleLine = bitsHigh(flags, DTF_SINGLE);

            uint8_t xAdv = 0, yAdv = 0, yAdvMax = 0;
            int8_t xOff = 0, yOff = 0, yOffMin = 0;
            Extent xAccum = 0, yAccum = rect.top + WindowYPadding;
            const Extent xExtent = rect.right - (WindowXPadding * 2);
            const char* cursor = text;

            while (*cursor != '\0') {
                xAccum = rect.left + WindowXPadding;
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
                        WindowTextSize,
                        WindowTextSize,
                        WindowTextFont
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
                const Extent drawnWidth = xAccum - rect.left + WindowXPadding;
                xAccum = xCenter
                    ? rect.left + (rect.width() / 2) - (drawnWidth / 2)
                    : rect.left + WindowXPadding;
                while (old_cursor < cursor) {
                    _gfx->drawChar(xAccum, yAccum, *old_cursor++, WindowTextColor,
                        WindowTextColor, WindowTextSize);
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
            _gfx->drawRoundRect(rect.left, rect.top, rect.width(), rect.height(),
                ButtonCornerRadius, pressed ? ButtonFrameColorPressed : ButtonFrameColor);
        }

        void drawButtonBackground(bool pressed, const Rect& rect) const final
        {
            _gfx->fillRoundRect(rect.left, rect.top, rect.width(), rect.height(),
                ButtonCornerRadius, pressed ? ButtonBgColorPressed : ButtonBgColor);
        }

        void drawButtonLabel(const char* lbl, bool pressed, const Rect& rect) const final
        {
            int16_t x, y;
            uint16_t width, height;
            _gfx->setTextSize(ButtonTextSize);
            _gfx->getTextBounds(lbl, rect.left, rect.top, &x, &y, &width, &height);
            _gfx->setCursor(
                x + (rect.width() / 2) - (width / 2),
                rect.top + (rect.height() / 2) + WindowTextYOffset
            );
            _gfx->setTextColor(pressed ? ButtonLabelColorPressed : ButtonLabelColor);
            _gfx->print(lbl);
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

    class IWindow
    {
    public:
        virtual std::shared_ptr<IWindow> getParent() const = 0;
        virtual void setParent(const std::shared_ptr<IWindow>& parent) = 0;

        virtual Rect getRect() const = 0;
        virtual void setRect(const Rect& rect) = 0;

        virtual Style getStyle() const = 0;
        virtual void setStyle(Style style) = 0;

        virtual WindowID getID() const = 0;

        virtual State getState() const = 0;
        virtual void setState(State state) = 0;

        virtual std::string getText() const = 0;
        virtual void setText(const std::string& text) = 0;

        virtual bool onCreate(MsgParam p1, MsgParam p2) = 0;
        virtual bool onDestroy(MsgParam p1, MsgParam p2) = 0;
        virtual bool onDraw(MsgParam p1, MsgParam p2) = 0;
        virtual bool onInput(MsgParam p1, MsgParam p2) = 0;
        virtual bool onEvent(MsgParam p1, MsgParam p2) = 0;
        virtual bool onResize(MsgParam p1, MsgParam p2) = 0;

        virtual bool routeMessage(Message msg, MsgParam p1 = 0, MsgParam p2 = 0) = 0;
        virtual void queueMessage(Message msg, MsgParam p1 = 0, MsgParam p2 = 0) = 0;
        virtual bool processQueue() = 0;
    };

    using WindowPtr = std::shared_ptr<IWindow>;

    class TWM : public std::enable_shared_from_this<TWM>
    {
    public:
        using WindowRegistry = std::map<WindowID, WindowPtr>;

        TWM() = delete;

        explicit TWM(const GfxDriverPtr& gfx, const ThemePtr& theme)
            : _gfx(gfx), _theme(theme)
        {
            TWM_ASSERT(_gfx);
            TWM_ASSERT(_theme);
            if (_theme) {
                _theme->setGfxDriver(_gfx);
            }
        }

        virtual ~TWM() = default;

        GfxDriverPtr getGfx() const { return _gfx; }
        ThemePtr getTheme() const { return _theme; }

        virtual void tearDown()
        {
# if !defined(TWM_SINGLETHREAD)
            ScopeLock lock(_regMtx);
# endif
            _registry.clear();
        }

        virtual void update()
        {
# if !defined(TWM_SINGLETHREAD)
            ScopeLock lock(_regMtx);
# endif
            if (_registry.empty() && _theme) {
                _theme->drawBlankScreen();
                return;
            }

            for (auto it : _registry) {
                // TODO: is this window completely covered by another, or off
                // the screen?
                if (bitsHigh(it.second->getStyle(), STY_VISIBLE) &&
                    bitsHigh(it.second->getState(), STA_ALIVE)) {
                    it.second->routeMessage(MSG_DRAW);
                }
                it.second->processQueue();
            }
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
# if !defined(TWM_SINGLETHREAD)
            ScopeLock lock(_regMtx);
# endif
            size_t numWindows = _registry.size();
            TWM_ASSERT(numWindows < std::numeric_limits<WindowID>::max() - 1);
            if (numWindows >= std::numeric_limits<WindowID>::max() - 1) {
                TWM_LOG(TWM_ERROR, "max window count exceeded");
                return nullptr;
            }
            auto it = _registry.find(id);
            if (it != _registry.end()) {
                TWM_LOG(TWM_ERROR, "duplicate %hhu", id);
                return nullptr;
            }
            Rect rect;
            rect.left = x;
            rect.top = y;
            rect.right = x + width;
            rect.bottom = y + height;
            std::shared_ptr<TWindow> win(
                std::make_shared<TWindow>(
                    shared_from_this(),
                    parent,
                    id,
                    style,
                    rect,
                    text
                )
            );
            if (!win) {
                TWM_LOG(TWM_ERROR, "alloc failed");
                return nullptr;
            }
            if (bitsHigh(style, STY_CHILD)) {
                if (!parent) {
                    TWM_LOG(TWM_ERROR, "STY_CHILD w/ null parent");
                    return nullptr;
                }
            }
            if (preCreateHook && !preCreateHook(win)) {
                TWM_LOG(TWM_ERROR, "pre-create hook false");
                return nullptr;
            }
            if (!win->routeMessage(MSG_CREATE)) {
                TWM_LOG(TWM_ERROR, "MSG_CREATE false");
                return nullptr;
            }
            win->setState(STA_ALIVE);
            if (bitsHigh(win->getStyle(), STY_AUTOSIZE)) {
                win->routeMessage(MSG_RESIZE);
            }
            if (bitsHigh(win->getStyle(), STY_VISIBLE)) {
                win->routeMessage(MSG_DRAW);
            }
            _registry[id] = win;
            TWM_LOG(TWM_DEBUG, "registered %hhu; count: %zu", id, _registry.size());
            return win;
        }

        template<class TPrompt>
        inline std::shared_ptr<TPrompt> createPrompt(
            const WindowPtr& parent,
            const std::string& text,
            const std::vector<std::pair<WindowID, std::string>>& buttons,
            const typename TPrompt::ResultCallback& callback
        )
        {
            auto prompt = createWindow<TPrompt>(
                parent,
                WID_PROMPT,
                STY_CHILD | STY_VISIBLE,
                _theme->getWindowXPadding(),
                _theme->getWindowYPadding(),
                _gfx->width() - (_theme->getWindowXPadding() * 2),
                _gfx->height() - (_theme->getWindowYPadding() * 2),
                text,
                [&](const std::shared_ptr<TPrompt>& win)
                {
                    for (const auto& btn : buttons) {
                        if (!win->addButton(btn.first, btn.second)) {
                            return false;
                        }
                    }
                    win->setResultCallback(callback);
                    return true;
                }
            );
            return prompt;
        }

        WindowPtr findWindow(WindowID id)
        {
# if !defined(TWM_SINGLETHREAD)
            ScopeLock lock(_regMtx);
# endif
            auto it = _registry.find(id);
            return it != _registry.end() ? it->second : nullptr;
        }

        bool destroyWindow(WindowID id)
        {
# if !defined(TWM_SINGLETHREAD)
            ScopeLock lock(_regMtx);
# endif
            auto it = _registry.find(id);
            if (it != _registry.end()) {
                if (it->second) {
                    std::vector<WindowRegistry::iterator> children;
                    for (auto it2 = _registry.begin(); it2 != _registry.end(); it2++) {
                        if (it2->second && it2->second->getParent() == it->second) {
                            children.push_back(it2);
                            it2->second->queueMessage(MSG_DESTROY);
                            it2->second->setState(it2->second->getState() & ~STA_ALIVE);
                        }
                    }
                    it->second->queueMessage(MSG_DESTROY);
                    it->second->setState(it->second->getState() & ~STA_ALIVE);
                    for (auto it2 : children) {
                        WindowID idChild = it2->first;
                        _registry.erase(it2);
                        TWM_LOG(TWM_DEBUG, "destroyed child %hhu (%hhu)", idChild, id);
                    }
                    Rect dirtyRect = it->second->getRect();
                    _registry.erase(it);
                    TWM_LOG(TWM_DEBUG, "destroyed %hhu; count: %zu", id, _registry.size());
                    for (auto it : _registry) {
                        if (it.second) {
                            Rect curRect = it.second->getRect();
                            if (curRect.overlaps(dirtyRect)) {
                                it.second->queueMessage(MSG_DRAW);
                            }
                        }
                    }
                    return true;
                }
            }
            return false;
        }

        void hitTest(Coord x, Coord y)
        {
# if !defined(TWM_SINGLETHREAD)
            ScopeLock lock(_regMtx);
# endif
            //TWM_LOG(TWM_DEBUG, "hit test @ %hd, %hd...", x, y);
            for (auto it = _registry.rbegin(); it != _registry.rend(); it++) {
                if (it->second) {
                    if (!bitsHigh(it->second->getStyle(), STY_VISIBLE) ||
                        !bitsHigh(it->second->getState(), STA_ALIVE)) {
                        continue;
                    }
                    auto rect = it->second->getRect();
                    if (rect.isPointWithin(Point(x, y))) {
                        InputParams params;
                        params.type = INPUT_TAP;
                        params.x    = x;
                        params.y    = y;
                        if (it->second->routeMessage(MSG_INPUT,
                            reinterpret_cast<MsgParam>(&params))) {
                            //TWM_LOG(TWM_DEBUG, "%hhu claimed hit test @ %hd, %hd",
                            //    win->getID(), x, y);
                            break;
                        }
                    }
                }
            }
        }

    protected:
        WindowRegistry _registry;
# if !defined(TWM_SINGLETHREAD)
        Mutex _regMtx;
# endif
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
            const std::shared_ptr<IWindow>& parent,
            WindowID id,
            Style style,
            const Rect& rect,
            const std::string& text
        ) : _wm(wm), _parent(parent), _style(style), _rect(rect), _id(id), _text(text)
        {
        }

        virtual ~Window() = default;

        std::shared_ptr<IWindow> getParent() const override { return _parent; }
        void setParent(const std::shared_ptr<IWindow>& parent) override { _parent = parent; }

        Rect getRect() const override { return _rect; }
        void setRect(const Rect& rect) override { _rect = rect; }

        Style getStyle() const override { return _style; }
        void setStyle(Style style) override { _style = style; }

        WindowID getID() const override { return _id; }

        State getState() const override { return _state; }
        void setState(State state) override { _state = state; }

        std::string getText() const override { return _text; }
        void setText(const std::string& text) override { _text = text; }

        /** MSG_CREATE: param1 = nullptr, param2 = nullptr. */
        bool onCreate(MsgParam p1, MsgParam p2) override { return true; }
        bool onDestroy(MsgParam p1, MsgParam p2) override { return true; }

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

        /** MSG_INPUT: param1 = InputParams*, param2 = nullptr. Return value
         * indicates whether or not the input event was consumed by this window. */
        bool onInput(MsgParam p1, MsgParam p2) override { return false; }

        /** MSG_EVENT: param1 = EventType, param2 = child WindowID. */
        bool onEvent(MsgParam p1, MsgParam p2) override { return true; }

        bool onResize(MsgParam p1, MsgParam p2) override
        {
            TWM_ASSERT(bitsHigh(getStyle(), STY_AUTOSIZE));
            return false;
        }

        bool routeMessage(Message msg, MsgParam p1 = 0, MsgParam p2 = 0) override
        {
            switch (msg) {
                case MSG_CREATE:  return onCreate(p1, p2);
                case MSG_DESTROY: return onDestroy(p1, p2);
                case MSG_DRAW:    return onDraw(p1, p2);
                case MSG_INPUT:   return onInput(p1, p2);
                case MSG_EVENT:   return onEvent(p1, p2);
                case MSG_RESIZE:  return onResize(p1, p2);
                default:
                    TWM_ASSERT(!"unknown message");
                return false;
            }
        }

        void queueMessage(Message msg, MsgParam p1 = 0, MsgParam p2 = 0) override
        {
# if !defined(TWM_SINGLETHREAD)
            ScopeLock lock(_queueMtx);
# endif
            PackagedMessage pm;
            pm.msg = msg;
            pm.p1 = p1;
            pm.p2 = p2;
            _queue.push(pm);
        }

        bool processQueue() override
        {
# if !defined(TWM_SINGLETHREAD)
            ScopeLock lock(_queueMtx);
# endif
            if (!_queue.empty()) {
                auto pm = _queue.front();
                _queue.pop();
                return routeMessage(pm.msg, pm.p1, pm.p2);
            }
            return false;
        }

    protected:
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
        PackagedMessageQueue _queue;
# if !defined(TWM_SINGLETHREAD)
        Mutex _queueMtx;
# endif
        TWMPtr _wm;
        std::shared_ptr<IWindow> _parent;
        Rect _rect;
        Style _style = 0;
        WindowID _id = 0;
        State _state = 0;
        std::string _text;
    };

    class Button : public Window
    {
    public:
        static constexpr u_long TappedDurationMsec = 100;

        using Window::Window;
        Button() = default;
        virtual ~Button() = default;

        virtual void onTapped()
        {
            _lastTapped = millis();
            TWM_ASSERT(_parent);
            if (_parent) {
                _parent->queueMessage(
                    MSG_EVENT,
                    EVT_CHILD_TAPPED,
                    getID()
                );
            }
        }

        bool onDraw(MsgParam p1, MsgParam p2) override
        {
            auto theme = _getTheme();
            if (theme) {
                bool pressed = (millis() - _lastTapped < TappedDurationMsec);
                Rect rect = getRect();
                theme->drawButtonBackground(pressed, rect);
                theme->drawButtonFrame(pressed, rect);
                theme->drawButtonLabel(getText().c_str(), pressed, rect);
                return true;
            }
            return false;
        }

        bool onInput(MsgParam p1, MsgParam p2) override
        {
            InputParams* ip = reinterpret_cast<InputParams*>(p1);
            TWM_ASSERT(ip != nullptr);
            if (ip != nullptr && ip->type == INPUT_TAP) {
                onTapped();
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
                rect.right = rect.left + max(width, theme->getButtonWidth())  + (theme->getButtonLabelPadding() * 2);
                rect.bottom = rect.top + theme->getButtonHeight();
                setRect(rect);
TODO_if_not_autosize_clip_label:
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
                theme->drawWindowText(getText().c_str(), DrawTextFlags, rect);
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
                theme->drawWindowText(getText().c_str(), DrawTextFlags, rect);
                return true;
            }
            return false;
        }
    };

    class Prompt : public Window
    {
    public:
        using ResultCallback = std::function<void(WindowID)>;

        using Window::Window;
        Prompt() = default;
        virtual ~Prompt() = default;

        void setResultCallback(const ResultCallback& callback)
        {
            _callback = callback;
        }

        bool addButton(WindowID id, const std::string& label)
        {
            if (_buttons.size() >= 2) {
                TWM_LOG(TWM_ERROR, "max 2 prompt buttons");
                return false;
            }
            auto wm = _getWM();
            if (wm) {
                auto theme = _getTheme();
                if (theme) {
                    auto btn = wm->createWindow<Button>(
                        shared_from_this(),
                        id,
                        STY_CHILD | STY_VISIBLE | STY_AUTOSIZE,
                        0,
                        0,
                        0,
                        0,
                        label
                    );
                    if (btn) {
                        _buttons.push_back(btn);
                        return true;
                    }
                }
            }
            return false;
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
                        STY_CHILD | STY_VISIBLE,
                        rect.top + theme->getWindowXPadding(),
                        rect.left + theme->getWindowYPadding(),
                        rect.width() - theme->getWindowXPadding() * 2,
                        rect.height() - ((theme->getWindowYPadding() * 3) + theme->getButtonHeight()),
                        getText()
                    );
                    if (!_label) {
                        return false;
                    }
                    Rect rectLbl = _label->getRect();
                    uint8_t idx = 0;
                    for (auto& btn : _buttons) {
                        Rect rectBtn = btn->getRect();
                        rectBtn.top = rectLbl.bottom + theme->getWindowYPadding();
                        rectBtn.bottom = rectBtn.top + theme->getButtonHeight();
                        auto width = rectBtn.width();
                        if (idx == 0) {
                            rectBtn.left = rect.left + theme->getWindowXPadding();
                            rectBtn.right = rectBtn.left + width;
                        } else {
                            rectBtn.right = rect.right - theme->getWindowXPadding();
                            rectBtn.left = rectBtn.right - width;
                        }
                        btn->setRect(rectBtn);
                        idx++;
                    }
                    return true;
                }
            }
            return false;
        }

        bool onEvent(MsgParam param1, MsgParam param2) override
        {
            switch (static_cast<EventType>(param1)) {
                case EVT_CHILD_TAPPED: {
                    if (_callback) {
                        _callback(static_cast<WindowID>(param2));
                    }
                    auto wm = _getWM();
                    if (wm) {
                        wm->destroyWindow(getID());
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
        std::vector<WindowPtr> _buttons;
        ResultCallback _callback;
    };
} // namespace twm

#endif // !_TWM_H_INCLUDED
