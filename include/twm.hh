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
# include <memory>
# include <mutex>
# include <deque>
# include <map>

# include <Adafruit_GFX.h>

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
    TWM_DEBUG = 2,
} _LoggingLevels;

#  define TWM_LOG(lvl, fmt, ...) \
    do { \
        switch(lvl) { \
            case TWM_ERROR: \
                Serial.printf("[E]: " fmt "\n" __VA_OPT__(,) __VA_ARGS__); \
                break; \
            case TWM_DEBUG: \
                Serial.printf("[D]: " fmt "\n" __VA_OPT__(,) __VA_ARGS__); \
                break; \
            default: \
                Serial.printf(fmt __VA_OPT__(,) __VA_ARGS__); \
                break; \
        } \
    } while(false)
# else
#  define TWM_LOG(lvl, fmt, ...)
# endif

# define TWM_ASSERT(expr) \
    do { \
        if (!(expr)) { \
            TWM_LOG(TWM_ERROR, "assert! '" #expr "' at %s:%d\n", __FILE__, __LINE__); \
        } \
    } while (false)

namespace twm
{
    /** For now, the only low-level graphics interface supported is Adafruit's. */
    using GfxInterface = Adafruit_GFX;

    /** Window identifier. */
    using WindowID = uint8_t;

    /** The invalid Window identifier. */
    static constexpr WindowID WID_INVALID = 0xff;

    /** Window style. */
    using Style = uint16_t;

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

        inline bool overlaps(const Rect& other) const noexcept
        {
            if (left >= other.left || right <= other.right) {
                if ((top < other.top && top + height() >= other.top) ||
                    (bottom > other.bottom && bottom - height() <= other.bottom) ||
                    (top >= other.top && bottom <= other.bottom)) {
                    return true;
                }
            }
            if (top >= other.top || bottom <= other.bottom) {
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
    inline bool areBitsHigh(const T1& bitmask, T2 bits)
    {
        return (bitmask & bits) == bits;
    }

    using Mutex     = std::mutex;
    using MutexLock = std::lock_guard<Mutex>;

    typedef enum
    {
        MSG_CREATE  = 1,
        MSG_DESTROY = 2,
        MSG_DRAW    = 3,
        MSG_INPUT   = 4
    } Message;

    typedef enum
    {
        WSF_VISIBLE   = 1 << 0,
        WSF_CHILD     = 1 << 1,
        WSF_MODAL     = 1 << 2,
        WSF_STAYONTOP = 1 << 3
    } _Style;

    class Window
    {
    public:
        Window() = default;

        Window(const std::shared_ptr<Window>& parent, WindowID id, Style style,
            const Rect& rect)
            : _parent(parent), _style(style), _rect(rect), _id(id) { }

        virtual ~Window() = default;

        std::shared_ptr<Window> getParent() const { return _parent.lock(); }
        void setParent(const std::shared_ptr<Window>& parent) { _parent = parent; }

        Style getStyle() const { return _style; }
        void setStyle(Style style) { _style = style; }

        Rect getRectangle() const { return _rect; }
        void setRectangle(const Rect& rect) { _rect = rect; }

        WindowID getID() const { return _id; }

        virtual void onCreate(void* param1, void* param2) { }
        virtual void onDestroy(void* param1, void* param2) { }
        virtual void onDraw(void* param1, void* param2) { }
        virtual void onInput(void* param1, void* param2) { }

        virtual void routeMessage(Message msg, void* param1 = nullptr,
            void* param2 = nullptr)
        {
            switch (msg) {
                case MSG_CREATE:  onCreate(param1, param2);  break;
                case MSG_DESTROY: onDestroy(param1, param2); break;
                case MSG_DRAW:    onDraw(param1, param2);    break;
                case MSG_INPUT:   onInput(param1, param2);   break;
            }
        }

    private:
        std::weak_ptr<Window> _parent;
        Rect _rect;
        Style _style;
        WindowID _id = 0;
    };

    using WindowPtr = std::shared_ptr<Window>;

    typedef enum {
        INPUT_TAP = 1
    } InputType;

    struct InputParams
    {
        InputType type;
        Coord x;
        Coord y;
    };

    template<class TImpl>
    class TWM : public TImpl
    {
    public:
        using WindowRegistry = std::map<WindowID, WindowPtr>;

        template<typename... TArgs>
        TWM(TArgs&& ...args) : TImpl(args...) { }

        virtual ~TWM() = default;

        static std::shared_ptr<TWM<TImpl>>& getInstance()
        {
            TWM_ASSERT(_instance != nullptr);
            return _instance;
        }

        template<typename... TArgs>
        static std::shared_ptr<TWM<TImpl>> init(TArgs&& ...args)
        {
            static_assert(std::is_base_of<GfxInterface, TImpl>::value);
            static std::once_flag flag;
            std::call_once(flag, [&]()
            {
                _instance.reset(new TWM<TImpl>(args...));
            });
            TWM_ASSERT(_instance != nullptr);
            return _instance;
        }

        virtual void tearDown()
        {
# if !defined(TWM_SINGLETHREAD)
            MutexLock lock(_reglock);
# endif
            _registry.clear();
        }

        virtual void update()
        {
# if !defined(TWM_SINGLETHREAD)
            MutexLock lock(_reglock);
# endif
            for (auto it : _registry) {
                // TODO: is this window covered by another, off
                // the screen, or hidden?
                if (it.second) {
                    auto style = it.second->getStyle();
                    if (!areBitsHigh(style, WSF_VISIBLE)) {
                        continue;
                    }
                    //TWM_LOG(TWM_DEBUG, "sending MSG_DRAW to window %hhu", it.first);
                    it.second->routeMessage(MSG_DRAW, this);
                }
            }
        }

        template<class TWindow>
        inline std::shared_ptr<TWindow> createWindow(const WindowPtr& parent,
            Style style, Coord x, Coord y, Extent width, Extent height)
        {
# if !defined(TWM_SINGLETHREAD)
            MutexLock lock(_reglock);
# endif
            TWM_ASSERT(_idCounter < std::numeric_limits<WindowID>::max() - 1);
            if (_idCounter >= std::numeric_limits<WindowID>::max() - 1) {
                TWM_LOG(TWM_ERROR, "max window count exceeded");
                return nullptr;
            }
            if (areBitsHigh(style, WSF_CHILD) && !parent) {
                TWM_LOG(TWM_ERROR, "window with child style has null parent");
                return nullptr;
            }
            WindowID id = ++_idCounter;
            Rect rect;
            rect.left = x;
            rect.top = y;
            rect.right = x + width;
            rect.bottom = y + height;
            std::shared_ptr<TWindow> win(std::make_shared<TWindow>(parent, id,
                style, rect));
            if (!win) {
                TWM_LOG(TWM_ERROR, "memory alloc failed");
                return nullptr;
            }
            win->routeMessage(MSG_CREATE, this);
            if (areBitsHigh(win->getStyle(), WSF_VISIBLE)) {
                win->routeMessage(MSG_DRAW, this);
            }
            _registry[id] = win;
            TWM_LOG(TWM_DEBUG, "added window %hhu to registry; count: %zu", id,
                _registry.size());
            return win;
        }

        WindowPtr findWindow(WindowID id) const
        {
# if !defined(TWM_SINGLETHREAD)
            MutexLock lock(_reglock);
# endif
            auto it = _registry.find(id);
            return it != _registry.end() ? it->second : nullptr;
        }

        bool destroyWindow(WindowID id)
        {
# if !defined(TWM_SINGLETHREAD)
            MutexLock lock(_reglock);
# endif
            auto it = _registry.find(id);
            if (it != _registry.end()) {
                if (it->second) {
                    it->second->routeMessage(MSG_DESTROY, this);
                    _registry.erase(it);
                    --_idCounter;
                    TWM_LOG(TWM_DEBUG, "destroyed window %hhu; count: %zu", id);
                    return true;
                }
            }
            return false;
        }

        bool hitTest(Coord x, Coord y)
        {
# if !defined(TWM_SINGLETHREAD)
            MutexLock lock(_reglock);
# endif
            TWM_LOG(TWM_DEBUG, "hit test @ %hd/%hd...", x, y);
            for (auto it = _registry.rbegin(); it != _registry.rend(); it++) {
                if (it->second) {
                    if (!areBitsHigh(it->second->getStyle(), WSF_VISIBLE)) {
                        continue;
                    }
                    auto rect = it->second->getRectangle();
                    TWM_LOG(TWM_DEBUG, "window %hhu rect: {%hd, %hd, %hd, %hd}",
                        it->first, rect.left, rect.top, rect.right, rect.bottom);
                    if (rect.isPointWithin(Point(x, y))) {
                        TWM_LOG(TWM_DEBUG, "hit!");
                        InputParams params;
                        params.type = INPUT_TAP;
                        params.x    = x;
                        params.y    = y;
                        it->second->routeMessage(MSG_INPUT, &params);
                        return true;
                    }
                }
            }
            return false;
        }

    protected:
        WindowID _idCounter = 0;
        WindowRegistry _registry;
# if !defined(TWM_SINGLETHREAD)
        Mutex _reglock;
# endif
        static std::shared_ptr<TWM<TImpl>> _instance;
    };

    template<class TImpl>
    std::shared_ptr<TWM<TImpl>> TWM<TImpl>::_instance;
} // namespace twm

#endif // !_TWM_H_INCLUDED
