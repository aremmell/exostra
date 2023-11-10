#ifndef _TWM_GFX_DRIVER_H_INCLUDED
#define _TWM_GFX_DRIVER_H_INCLUDED

#include <platform.hh>

namespace twm
{
    /**
     * Abstract low-level graphics driver interface.
     */
    class GfxDriver
    {
    public:
        /**
         * Allocate resources and prepare to render to the display.
         * @return true if the driver was initialized successfully, false otherwise.
         */
        virtual bool init() = 0;

        /**
         * Deallocate resources and reset internal state to uninitialized.
         */
        virtual void tearDown() = 0;
    };
} // namespace twm


#endif // !_TWM_GFX_DRIVER_H_INCLUDED
