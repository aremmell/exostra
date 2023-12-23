# exostra

Exostra Window Manager[^1]

A single-header (C++11) [compositing window manager](https://en.wikipedia.org/wiki/Compositing_window_manager) designed specifically for use with small touch displays, such as the kind you might utilize while building an IoT project.

Currently, exostra is designed to be consumed by an Arduino/PlatformIO/ESP-IDF project as a library dependency, and has support for the following low-level graphics libraries/interfaces:

- [Adafruit GFX](https://github.com/adafruit/Adafruit-GFX-Library) (via `Adafruit_SPITFT` and `GFXcanvas16`)
- [Adafruit RA8875](https://github.com/adafruit/Adafruit_RA8875)
- [Arduino GFX Library](https://github.com/moononournation/Arduino_GFX) (via various display-specific classes and `Arduino_Canvas`)

## Notable features

- Single-header implementation (C++11)
- Only draws pixels that must be redrawn and will be visible; rendering is extremely fast: with several top-level windows and many widgets, I am getting ~50 _microsecond_ average rendering times (on an Unexpected Maker ProS3 connected to an Adafruit HX8357D via EYESPI)!
- Uses templates to abstract the low-level graphics library away, allowing the underlying graphics library to be swapped out with 1-2 lines of changes.
- Themeable. A default theme is under development along with the library, but themeing is extremely simple through the use of inheritance/virtual functions and templates.
- Automatically adapts the scale and spacing of windows/widgets based on the display size and resolution.
- Screensaver! I've added the ability for a screensaver to appear after a given amount of time with no user interaction in order to a) conserve power, and b) to prevent burn-in on certain displays. Right now it's just a blank screen, but maybe I'll add some graphics later on.

## Current progress

1. WIP/not ready for production use. I have only written the basic window classes like button, label, progress bar, prompt (message box), checkbox, etc. as of now, but stay tuned!
2. Limitations (some to be resolved, some perhaps not):
  - Only supports 16-bit RGB 565 color mode (I will be adding 24-bit RGB support as well as translation from 24-bit to 16-bit)
  - Requires a not-insignificant amount of heap memory, as each top-level window is paired with a 16bpp off-screen buffer which is shared with all descendants of the window. Using these off-screen buffers allows exostra to copy the raw pixel data directly to the display hardware with zero flickering. Depending on the resolution of display and number of top-level windows, these buffers may consume several hundred KiB of heap memory. I am considering providing alternate modes, such as one that reuses a single screen-sized off-screen buffer for all windows, which will be much slower to render, but use less resources. Another possibility is direct rendering to the display hardware, which will result in flickering/noticeable delays, but could allow exostra to run on boards it could otherwise not run on.
  - Only processes tap events. I have not gotten to swiping/multi-touch gestures yet.

I will upload a sample video in the weeks to come, as I have more useful features to show off.

[^1]: Exostra (n): A relatively obscure Latin term which means "theatrical machine" or "revealing the inside of a house to spectators." _(Charles Beard, “Cassell’s Latin Dictionary”, 1892)_
