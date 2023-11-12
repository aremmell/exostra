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
# include <vector>
# include <queue>
# include <mutex>
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

namespace twm
{
    /** For now, the only low-level graphics interface supported is Adafruit's. */
    using GfxInterface = Adafruit_GFX;

    /** Window identifier. */
    using WindowID = uint8_t;

    /** Window style. */
    using Style = uint16_t;

    /** Window state. */
    using State = uint16_t;

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
    inline bool bitsHigh(const T1& bitmask, T2 bits)
    {
        return (bitmask & bits) == bits;
    }

    using Mutex     = std::mutex;
    using MutexLock = std::lock_guard<Mutex>;

    typedef enum
    {
        MSG_NONE    = 0,
        MSG_CREATE  = 1,
        MSG_DESTROY = 2,
        MSG_DRAW    = 3,
        MSG_INPUT   = 4
    } Message;

    typedef enum
    {
        STY_VISIBLE   = 1 << 0,
        STY_CHILD     = 1 << 1,
        STY_MODAL     = 1 << 2,
        STY_STAYONTOP = 1 << 3
    } _WindowStyleFlags;

    typedef enum
    {
        STA_ALIVE = 1 << 0
    } _WindowStateFlags;

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

    class Theme
    {
    public:
        virtual void drawWindowFrame(GfxInterface* gfx, const Rect& rect) = 0;
        virtual void drawWindowBackground(GfxInterface* gfx, const Rect& rect) = 0;
        virtual void drawButtonFrame(GfxInterface* gfx, const Rect& rect) = 0;
        virtual void drawButtonBackground(GfxInterface* gfx, const Rect& rect) = 0;
        virtual void drawButtonLabel(GfxInterface* gfx, const Rect& rect) = 0;
    };

    using ThemePtr = std::shared_ptr<Theme>;

    class DefaultTheme : public Theme
    {
    public:
        static constexpr Extent WindowFrameThickness = 1;
        static constexpr uint16_t WindowFrameColor = 0x7bef;
        static constexpr uint16_t WindowBgColor = 0xc618;

        void drawWindowFrame(GfxInterface* gfx, const Rect& rect)
        {
            Rect tmp = rect;
            tmp.deflate(WindowFrameThickness);
            gfx->drawRect(tmp.left, tmp.top, tmp.width(), tmp.height(),
                WindowFrameColor);
        }

        void drawWindowBackground(GfxInterface* gfx, const Rect& rect)
        {
            gfx->fillRect(rect.left, rect.top, rect.width(), rect.height(),
                WindowBgColor);
        }

        void drawButtonFrame(GfxInterface* gfx, const Rect& rect)
        {
        }

        void drawButtonBackground(GfxInterface* gfx, const Rect& rect)
        {
        }

        void drawButtonLabel(GfxInterface* gfx, const Rect& rect)
        {
        }
    };

    struct PackagedMessage
    {
        Message msg  = MSG_NONE;
        void* param1 = nullptr;
        void* param2 = nullptr;
    };

    using PackagedMessageQueue = std::queue<PackagedMessage>;

    class Window
    {
    public:
        Window() = default;

        Window(const ThemePtr& theme, const std::shared_ptr<Window>& parent,
            WindowID id, Style style, const Rect& rect)
            : _theme(theme), _parent(parent), _style(style), _rect(rect), _id(id) { }

        virtual ~Window() = default;

        std::shared_ptr<Window> getParent() const { return _parent; }
        void setParent(const std::shared_ptr<Window>& parent) { _parent = parent; }

        Style getStyle() const { return _style; }
        void setStyle(Style style) { _style = style; }

        State getState() const { return _state; }
        void setState(State state) { _state = state; }

        Rect getRect() const { return _rect; }
        void setRect(const Rect& rect) { _rect = rect; }

        WindowID getID() const { return _id; }

        virtual bool onCreate(void* param1, void* param2) { return true; }
        virtual bool onDestroy(void* param1, void* param2) { return true; }

        /** MSG_DRAW: param1 = GfxInterface*, param2 = nullptr*/
        virtual bool onDraw(void* param1, void* param2)
        {
            GfxInterface* gfx = static_cast<GfxInterface*>(param1);
            TWM_ASSERT(gfx != nullptr && _theme);
            if (gfx != nullptr && _theme) {
                _theme->drawWindowBackground(gfx, getRect());
                _theme->drawWindowFrame(gfx, getRect());
                return true;
            }
            return false;
        }

        /** MSG_INPUT: param1 = InputParams*, param2 = nullptr. Return value
         * indicates whether or not the input event was consumed by this window. */
        virtual bool onInput(void* param1, void* param2) { return false; }

        bool routeMessage(Message msg, void* param1 = nullptr,
            void* param2 = nullptr)
        {
            switch (msg) {
                case MSG_CREATE:  return onCreate(param1, param2);
                case MSG_DESTROY: return onDestroy(param1, param2);
                case MSG_DRAW:    return onDraw(param1, param2);
                case MSG_INPUT:   return onInput(param1, param2);
                default:
                    TWM_ASSERT(!"unknown message");
                return false;
            }
        }

        void queueMessage(Message msg, void* param1 = nullptr,
            void* param2 = nullptr)
        {
# if !defined(TWM_SINGLETHREAD)
            MutexLock lock(_queueLock);
# endif
            PackagedMessage pm;
            pm.msg = msg;
            pm.param1 = param1;
            pm.param2 = param2;
            _queue.push(pm);
        }

        bool processQueue()
        {
# if !defined(TWM_SINGLETHREAD)
            MutexLock lock(_queueLock);
# endif
            if (!_queue.empty()) {
                auto pm = _queue.front();
                _queue.pop();
                return routeMessage(pm.msg, pm.param1, pm.param2);
            }
            return false;
        }

    private:
        PackagedMessageQueue _queue;
        Mutex _queueLock;
        ThemePtr _theme;
        std::shared_ptr<Window> _parent;
        Rect _rect;
        Style _style = 0;
        WindowID _id = 0;
        State _state = 0;
    };

    using WindowPtr = std::shared_ptr<Window>;

    template<class TImpl, class TTheme = DefaultTheme>
    class TWM : public TImpl
    {
    public:
        using WindowRegistry = std::map<WindowID, WindowPtr>;

        template<typename... TArgs>
        TWM(TArgs&& ...args) : TImpl(args...) { }

        virtual ~TWM() = default;

        static std::shared_ptr<TWM<TImpl, TTheme>>& getInstance()
        {
            TWM_ASSERT(_instance != nullptr);
            return _instance;
        }

        template<typename... TArgs>
        static std::shared_ptr<TWM<TImpl, TTheme>> init(TArgs&& ...args)
        {
            static_assert(std::is_base_of<GfxInterface, TImpl>::value);
            static std::once_flag flag;
            std::call_once(flag, [&]()
            {
                _instance.reset(new TWM<TImpl, TTheme>(args...));
                TWM_ASSERT(_instance != nullptr);
                _theme.reset(new TTheme());
                TWM_ASSERT(_theme != nullptr);
            });
            return _instance;
        }

        virtual void tearDown()
        {
# if !defined(TWM_SINGLETHREAD)
            MutexLock lock(_regLock);
# endif
            _registry.clear();
        }

        virtual void update()
        {
# if !defined(TWM_SINGLETHREAD)
            MutexLock lock(_regLock);
# endif
            if (_registry.empty()) {
                TImpl::fillScreen(0x0000);
                return;
            }

            for (auto it : _registry) {
                // TODO: is this window completely covered by another, or off
                // the screen?
                if (it.second) {
                    if (bitsHigh(it.second->getStyle(), STY_VISIBLE) &&
                        bitsHigh(it.second->getState(), STA_ALIVE)) {
                        it.second->queueMessage(MSG_DRAW, this);
                    }
                    it.second->processQueue();
                }
            }
        }

        template<class TWindow>
        inline std::shared_ptr<TWindow> createWindow(const WindowPtr& parent,
            Style style, Coord x, Coord y, Extent width, Extent height)
        {
# if !defined(TWM_SINGLETHREAD)
            MutexLock lock(_regLock);
# endif
            TWM_ASSERT(_idCounter < std::numeric_limits<WindowID>::max() - 1);
            if (_idCounter >= std::numeric_limits<WindowID>::max() - 1) {
                TWM_LOG(TWM_ERROR, "max window count exceeded");
                return nullptr;
            }

            WindowID id = _idCounter + 1;
            Rect rect;
            rect.left = x;
            rect.top = y;
            rect.right = x + width;
            rect.bottom = y + height;
            std::shared_ptr<TWindow> win(std::make_shared<TWindow>(_theme, parent,
                id, style, rect));
            if (!win) {
                TWM_LOG(TWM_ERROR, "memory alloc failed");
                return nullptr;
            }
            if (bitsHigh(style, STY_CHILD)) {
                if (!parent) {
                    TWM_LOG(TWM_ERROR, "STY_CHILD but null parent");
                    return nullptr;
                }
            }
            if (!win->routeMessage(MSG_CREATE, this)) {
                TWM_LOG(TWM_ERROR, "MSG_CREATE ret false");
                return nullptr;
            }
            win->setState(STA_ALIVE);
            if (bitsHigh(win->getStyle(), STY_VISIBLE)) {
                if (!win->routeMessage(MSG_DRAW, this)) {
                    TWM_LOG(TWM_WARN, "MSG_DRAW ret false");
                }
            }
            ++_idCounter;
            _registry[id] = win;
            TWM_LOG(TWM_DEBUG, "added window %hhu to registry; count: %zu", id,
                _registry.size());
            return win;
        }

        WindowPtr findWindow(WindowID id) const
        {
# if !defined(TWM_SINGLETHREAD)
            MutexLock lock(_regLock);
# endif
            auto it = _registry.find(id);
            return it != _registry.end() ? it->second : nullptr;
        }

        bool destroyWindow(WindowID id)
        {
# if !defined(TWM_SINGLETHREAD)
            MutexLock lock(_regLock);
# endif
            auto it = _registry.find(id);
            if (it != _registry.end()) {
                if (it->second) {
                    std::vector<WindowRegistry::iterator> children;
                    for (auto it2 = _registry.begin(); it2 != _registry.end(); it2++) {
                        if (it2->second && it2->second->getParent() == it->second) {
                            children.push_back(it2);
                            it2->second->queueMessage(MSG_DESTROY, this);
                            it2->second->setState(it2->second->getState() & ~STA_ALIVE);
                        }
                    }
                    it->second->queueMessage(MSG_DESTROY, this);
                    it->second->setState(it->second->getState() & ~STA_ALIVE);
                    for (auto it2 : children) {
                        WindowID idChild = it2->first;
                        _registry.erase(it2);
                        --_idCounter;
                        TWM_LOG(TWM_DEBUG, "destroyed child window %hhu (%hhu)",
                            idChild, id);
                    }
                    Rect dirtyRect = it->second->getRect();
                    _registry.erase(it);
                    --_idCounter;
                    TWM_LOG(TWM_DEBUG, "destroyed window %hhu; count: %zu", id,
                        _registry.size());

                    for (auto it : _registry) {
                        if (it.second) {
                            Rect curRect = it.second->getRect();
                            if (curRect.overlaps(dirtyRect)) {
                                TWM_LOG(TWM_DEBUG, "redrawing window %hhu; rect"
                                    " (%hd, %hd, %hu, %hu) overlap with dirty rect"
                                    " (%hd, %hd, %hu, %hu)", it.second->getID(),
                                    curRect.left, curRect.top, curRect.right,
                                    curRect.bottom, dirtyRect.left, dirtyRect.top,
                                    dirtyRect.right, dirtyRect.bottom);
                                it.second->queueMessage(MSG_DRAW, this);
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
            if (!_regLock.try_lock()) {
                TWM_LOG(TWM_DEBUG, "mutex lock fail; bailing");
                return;
            }
# endif
            //TWM_LOG(TWM_DEBUG, "hit test @ %hd/%hd...", x, y);
            std::queue<WindowPtr> candidates;
            for (auto it = _registry.rbegin(); it != _registry.rend(); it++) {
                if (it->second) {
                    if (!bitsHigh(it->second->getStyle(), STY_VISIBLE) ||
                        !bitsHigh(it->second->getState(), STA_ALIVE)) {
                        continue;
                    }
                    auto rect = it->second->getRect();
                    //TWM_LOG(TWM_DEBUG, "window %hhu rect: {%hd, %hd, %hd, %hd}",
                    //    it->first, rect.left, rect.top, rect.right, rect.bottom);
                    if (rect.isPointWithin(Point(x, y))) {
                        //TWM_LOG(TWM_DEBUG, "hit!");
                        candidates.push(it->second);
                    }
                }
            }
            _regLock.unlock();
            while (!candidates.empty()) {
                InputParams params;
                params.type = INPUT_TAP;
                params.x    = x;
                params.y    = y;
                auto win = candidates.front();
                candidates.pop();
                if (win->routeMessage(MSG_INPUT, &params)) {
                    //TWM_LOG(TWM_DEBUG, "window %hhu claimed hit test @ %hd/%hd",
                    //    win->getID(), x, y);
                    break;
                }
            }
        }

    protected:
        WindowID _idCounter = 0;
        WindowRegistry _registry;
# if !defined(TWM_SINGLETHREAD)
        Mutex _regLock;
# endif
        static std::shared_ptr<TWM<TImpl, TTheme>> _instance;
        static std::shared_ptr<TTheme> _theme;
    };

    template<class TImpl, class TTheme>
    std::shared_ptr<TWM<TImpl, TTheme>> TWM<TImpl, TTheme>::_instance;

    template<class TImpl, class TTheme>
    std::shared_ptr<TTheme> TWM<TImpl, TTheme>::_theme;
} // namespace twm

#endif // !_TWM_H_INCLUDED
