# twm
Thumby Window Manager[^1]

A window management framework designed to run on microcontrollers that are attached to a touch-capable display.

1. WIP. I have only written the basic window classes like button, label, progress bar, prompt (message box), checkbox, etc. as of now, but stay tuned!
2. Currently building on top of Adafruit's GFX library, since it has a common interface for every display controller they support. I may decide to shim it out so that other graphics libraries can be plugged in instead.
3. Trying to keep it minimal, but my desktop C++ habits have caused some more bloat than I would like. I will need to rein that in. Maybe that's what I will do next.

## Notable features

- Single-header implementation (C++11)
- Uses templates to abstract the low-level graphics library away
- Themeable. I am currently developing with a theme that I suppose resembles Windows 98 or Serenity OS, but anything is possible by swapping out themes (also abstracted via templates).
- Automaticallyy adapts the size and spacing of controls based on the display size and resolution.

I will upload a sample video in the weeks to come, as I have more useful features to show off.

[^1]: Thumby: A word used in the early 1900s to describe either a clumsy person, or something that has been marked with grubby fingerprints.
