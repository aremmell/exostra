#include <Arduino.h>
#include <SPI.h>
#include <Wire.h>
#include <Arduino_GFX_Library.h>
#include <Adafruit_GFX.h>
#include <Adafruit_FT6206.h>
#include <Adafruit_CST8XX.h>

#define TFT_720_SQUARE
//#define TFT_480_ROUND
//#define TFT_320_RECTANGLE

#if defined(TFT_720_SQUARE)
# include <Fonts/FreeSans18pt7b.h>
# define TWM_DEFAULT_FONT &FreeSans18pt7b
# define TWM_DISPLAY_WIDTH 720
# define TWM_DISPLAY_HEIGHT 720
# define I2C_TOUCH_ADDR 0x48
# define TWM_USE_ARDUINO_GFX
#elif defined(TFT_480_ROUND)
# include <Fonts/FreeSans12pt7b.h>
# define TWM_DEFAULT_FONT &FreeSans12pt7b
# define TWM_DISPLAY_WIDTH 480
# define TWM_DISPLAY_HEIGHT 480
# define I2C_TOUCH_ADDR 0x15
# define TWM_USE_ARDUINO_GFX
#elif defined(TFT_320_RECTANGLE)
# include <Fonts/FreeSans9pt7b.h>
# define TWM_DEFAULT_FONT &FreeSans9pt7b
# define TWM_DISPLAY_WIDTH 240
# define TWM_DISPLAY_HEIGHT 320
# define TS_MINX 0
# define TS_MINY 0
# define TS_MAXX TWM_DISPLAY_WIDTH
# define TS_MAXY TWM_DISPLAY_HEIGHT
# define I2C_TOUCH_ADDR 0x38
# define TWM_USE_ADAFRUIT_GFX
# include <Adafruit_ILI9341.h>
#else
# error "invalid display selection"
#endif

#include "twm.hh"

// If no touches are registered in this time, paint the screen
// black as a pseudo-screensaver. In the future, save what was on
// the screen and restore it after.
#define TFT_TOUCH_TIMEOUT 60000

// The FT6206 uses hardware I2C (SCL/SDA)
Adafruit_FT6206 focal_ctp;
Adafruit_CST8XX cst_ctp;

#if defined(ARDUINO_PROS3)
# include <UMS3.h>
UMS3 ums3;
/**
 * Unexpected Maker ProS3 implied.
 * Pins: sda 8, scl 9, tcs 12, dc 13, rst 14
 */
# define TFT_CS 12
# define TFT_DC 13
#endif

using namespace thumby;

#if defined(TFT_320_RECTANGLE)
# if !defined(ARDUINO_PROS3)
#  error "only the ProS3 is configured for use with this display"
# endif

auto wm = createWindowManager(
    std::make_shared<Adafruit_ILI9341>(TFT_CS, TFT_DC),
    std::make_shared<IGfxContext>(TWM_DISPLAY_HEIGHT, TWM_DISPLAY_WIDTH),
    std::make_shared<DefaultTheme>(),
    TWM_DEFAULT_FONT
);
#else
/** Implied Qualia RGB666 for now. */
# if defined(PLATFORMIO) // TODO: detect qualia, hopefully remove hack
#  include <esp32_qualia.h>
#  define PIN_NS qualia
# else
#  define PIN_NS
# endif
Arduino_XCA9554SWSPI *expander = new Arduino_XCA9554SWSPI(
    PIN_NS::PCA_TFT_RESET, PIN_NS::PCA_TFT_CS, PIN_NS::PCA_TFT_SCK, PIN_NS::PCA_TFT_MOSI, &Wire, 0x3F
);
Arduino_ESP32RGBPanel *rgbpanel = new Arduino_ESP32RGBPanel(
    PIN_NS::TFT_DE, PIN_NS::TFT_VSYNC, PIN_NS::TFT_HSYNC, PIN_NS::TFT_PCLK,
    PIN_NS::TFT_R1, PIN_NS::TFT_R2, PIN_NS::TFT_R3, PIN_NS::TFT_R4, PIN_NS::TFT_R5,
    PIN_NS::TFT_G0, PIN_NS::TFT_G1, PIN_NS::TFT_G2, PIN_NS::TFT_G3, PIN_NS::TFT_G4, PIN_NS::TFT_G5,
    PIN_NS::TFT_B1, PIN_NS::TFT_B2, PIN_NS::TFT_B3, PIN_NS::TFT_B4, PIN_NS::TFT_B5,
    1 /* hsync_polarity */, 50 /* hsync_front_porch */, 2 /* hsync_pulse_width */, 44 /* hsync_back_porch */,
    1 /* vsync_polarity */, 16 /* vsync_front_porch */, 2 /* vsync_pulse_width */, 18 /* vsync_back_porch */
);
auto display = std::make_shared<Arduino_RGB_Display>(
  TWM_DISPLAY_WIDTH,
  TWM_DISPLAY_HEIGHT,
  rgbpanel,
  0,
  true,
  expander,
  GFX_NOT_DEFINED,
# if defined(TFT_720_SQUARE) // 4.0" 720x720 square display
  nullptr,
  0
# elif defined(TFT_480_ROUND) // 2.1" 480x480 round display
    TL021WVC02_init_operations,
    sizeof(TL021WVC02_init_operations)
# else
#  error "invalid display selection"
# endif
);
auto wm = createWindowManager(
    display,
    std::make_shared<IGfxContext>(TWM_DISPLAY_WIDTH, TWM_DISPLAY_HEIGHT, display.get()),
    std::make_shared<DefaultTheme>(),
    TWM_DEFAULT_FONT
);
#endif

class EveryDayNormalButton : public Button
{
public:
  using Button::Button;
  EveryDayNormalButton() = default;
  virtual ~EveryDayNormalButton() = default;

  void setLabel(const WindowPtr& label) { _label = label; }
  void setPrompt(const WindowPtr& prompt) { _prompt = prompt; }

  bool onTapped(Coord x, Coord y) override
  {
    Button::onTapped(x, y);
    if (_label) {
      _label->setText("Tapped!");
    }
    if (_prompt) {
      _prompt->show();
    }
    return true;
  }

private:
  WindowPtr _label;
  WindowPtr _prompt;
};

class DefaultWindow : public Window
{
public:
  using Window::Window;
  DefaultWindow() = default;
  virtual ~DefaultWindow() = default;
};

class TestLabel : public Label
{
public:
  using Label::Label;
  TestLabel() = default;
  virtual ~TestLabel() = default;
};

class TestProgressBar : public ProgressBar
{
public:
  using ProgressBar::ProgressBar;
  TestProgressBar() = default;
  virtual ~TestProgressBar() = default;
};

class TestCheckbox : public CheckBox
{
public:
  using CheckBox::CheckBox;
  TestCheckbox() = default;
  virtual ~TestCheckbox() = default;
};

class TestYesNoPrompt : public Prompt
{
public:
  using Prompt::Prompt;
  TestYesNoPrompt() = default;
  virtual ~TestYesNoPrompt() = default;
};

class TestOKPrompt : public Prompt
{
public:
  using Prompt::Prompt;
  TestOKPrompt() = default;
  virtual ~TestOKPrompt() = default;
};

std::shared_ptr<TestYesNoPrompt> yesNoPromptWnd;
std::shared_ptr<TestOKPrompt> okPrompt;
std::shared_ptr<TestProgressBar> testProgressBar;
std::shared_ptr<TestCheckbox> testCheckbox;

void on_fatal_error()
{
#if defined(ARDUINO_PROS3)
  ums3.setPixelPower(true); // assume pixel could be off.
  ums3.setPixelBrightness(255); // maximum brightness.
  while (true) {
    ums3.setPixelColor(0xff, 0x00, 0x00); // pure red.
    delay(1000);
    ums3.setPixelColor(0x00, 0x00, 0x00); // black (off).
    delay(1000);
  }
#else
  // TODO: blink an LED or something.
  Serial.println("fatal error");
  while (true);
#endif
}

bool touchInitialized = false;
bool isFocalTouch = false;

void setup(void)
{
  try {
    Serial.begin(115200);
    while (!Serial) {
      /// TODO: If you want to run without a serial cable, exit this loop
      delay(10);
    }

    delay(500);

    TWM_LOG(TWM_DEBUG, "begin setup");

  #if defined(TFT_320_RECTANGLE)
    ums3.begin();
  #else
  # ifdef GFX_EXTRA_PRE_INIT
    GFX_EXTRA_PRE_INIT();
  # endif
    expander->pinMode(PIN_NS::PCA_TFT_BACKLIGHT, OUTPUT);
    expander->digitalWrite(PIN_NS::PCA_TFT_BACKLIGHT, HIGH);
  #endif
    Wire.setClock(1000000);
    if (!wm->begin(3)) {
      Serial.println("WindowManager: error!");
      on_fatal_error();
    }
    Serial.println("WindowManager: OK");
    wm->getTheme()->drawDesktopBackground();
    if (!focal_ctp.begin(0, &Wire, I2C_TOUCH_ADDR)) {
      Serial.print("FT6206: error at 0x");
      Serial.println(I2C_TOUCH_ADDR, HEX);
      if (!cst_ctp.begin(&Wire, I2C_TOUCH_ADDR)) {
        Serial.print("CST8XX: error at 0x");
        Serial.println(I2C_TOUCH_ADDR, HEX);
      } else {
        touchInitialized = true;
        isFocalTouch = false;
        Serial.println("CST8XX: OK");
      }
    } else {
      touchInitialized = true;
      isFocalTouch = true;
      Serial.println("FT6206: OK");
    }

    if (!touchInitialized) {
      on_fatal_error();
    }

    auto xPadding = wm->getTheme()->getWindowXPadding();
    auto defaultWin = wm->createWindow<DefaultWindow>(
      nullptr,
      2,
      STY_VISIBLE,
      xPadding,
      xPadding,
      wm->getScreenWidth() - (xPadding * 2),
      wm->getScreenHeight() - (xPadding * 2)
    );
    if (!defaultWin) {
      on_fatal_error();
    }

    auto scaledValue = [&](Extent value)
    {
      return wm->getTheme()->getScaledValue(value);
    };

    auto x = xPadding * 2;
    auto y = scaledValue(50);
    auto button1 = wm->createWindow<EveryDayNormalButton>(defaultWin, 3,
      STY_CHILD | STY_VISIBLE | STY_AUTOSIZE | STY_BUTTON, x, y, 0, 0,
      "Button");
    if (!button1) {
      on_fatal_error();
    }

    auto cx = button1->getRect().width();
    auto cy = scaledValue(30);
    x = button1->getRect().right + xPadding;
    auto label1 = wm->createWindow<TestLabel>(defaultWin, 4, STY_CHILD | STY_VISIBLE | STY_LABEL,
      x, y, cx, cy, "Label");
    if (!label1) {
      on_fatal_error();
    }
    button1->setLabel(label1);

    auto yPadding = wm->getTheme()->getWindowYPadding();

    cy = wm->getTheme()->getProgressBarHeight();
    testProgressBar = wm->createProgressBar<TestProgressBar>(defaultWin, 5,
      STY_CHILD | STY_VISIBLE | STY_PROGBAR, xPadding * 2, button1->getRect().bottom + yPadding,
      defaultWin->getRect().width() - (xPadding * 2), cy, PBR_INDETERMINATE);
    if (!testProgressBar) {
      on_fatal_error();
    }

    x = defaultWin->getRect().left + xPadding;
    y = testProgressBar->getRect().bottom + yPadding;
    cx = scaledValue(130);
    cy = scaledValue(30);
    testCheckbox = wm->createWindow<TestCheckbox>(defaultWin, 8,
      STY_CHILD | STY_VISIBLE | STY_CHECKBOX, x, y, cx, cy, "CheckBox");
    if (!testCheckbox) {
      on_fatal_error();
    }

    okPrompt = wm->createPrompt<TestOKPrompt>(
      nullptr,
      6,
      STY_PROMPT,
      "You did a thing, and now this is on your screen.",
      {{100, "OK"}},
      [](WindowID id)
      {
        // NOOP.
      }
    );
    if (!okPrompt) {
      on_fatal_error();
    }

    yesNoPromptWnd = wm->createPrompt<TestYesNoPrompt>(
      nullptr,
      7,
      STY_PROMPT,
      "This is a test prompt. Please choose an option.",
      {{100, "Yes"}, {101, "No"}},
      [&](WindowID id)
      {
        std::string prompt = "You tapped the ";
        prompt += id == 100 ? "Yes" : "No";
        prompt += " button.";
        okPrompt->setText(prompt);
        okPrompt->show();
      }
    );
    if (!yesNoPromptWnd) {
      on_fatal_error();
    }
    button1->setPrompt(yesNoPromptWnd);
  } catch (const std::exception& ex) {
    TWM_LOG(TWM_ERROR, "exception: '%s'", ex.what());
  }
  TWM_LOG(TWM_DEBUG, "setup complete");
}

float curProgress = 0.0f;
u_long lastTouch = 0UL;
bool screensaverOn = false;

#if defined(TWM_RENDER_PERF)
u_long accumulator = 0UL;
u_long beginTime = 0UL;
uint8_t paintCount = 0;
#endif

void loop()
{
  try {
  #if defined(TFT_320_RECTANGLE)
      auto mapXCoord = [](Coord x)
      {
        return map(x, TS_MINX, TS_MAXX, TS_MAXX, TS_MINX);
      };
      auto mapYCoord = [](Coord y)
      {
        return map(y, TS_MINY, TS_MAXY, TS_MAXY, TS_MINY);
      };
      auto swapCoords = [&](Coord x, Coord y)
      {
        auto tmp = y;
        y = x;
        x = wm->getScreenWidth() - tmp;
        return std::make_pair(x, y);
      };
  #endif
    if (isFocalTouch && focal_ctp.touched()) {
      lastTouch = millis();
      if (screensaverOn) {
        screensaverOn = false;
      }
      TS_Point pt = focal_ctp.getPoint();
  #if defined(TFT_320_RECTANGLE)
      auto tmp = swapCoords(mapXCoord(pt.x), mapYCoord(pt.y));
      pt.x = tmp.first;
      pt.y = tmp.second;
  #endif
      wm->hitTest(pt.x, pt.y);
    } else if (!isFocalTouch && cst_ctp.touched()) {
      lastTouch = millis();
      if (screensaverOn) {
        screensaverOn = false;
      }
      CST_TS_Point pt = cst_ctp.getPoint();
  #if defined(TFT_320_RECTANGLE)
      auto tmp = swapCoords(mapXCoord(pt.x), mapYCoord(pt.y));
      pt.x = tmp.first;
      pt.y = tmp.second;
  #endif
      wm->hitTest(pt.x, pt.y);
    } else {
      if (!screensaverOn && millis() - lastTouch > TFT_TOUCH_TIMEOUT) {
        // TODO: refactor
        wm->getTheme()->drawScreensaver();
        screensaverOn = true;
      }
    }
    if (!screensaverOn) {
      if (curProgress < 100.0f) {
        curProgress += wm->getTheme()->getProgressBarIndeterminateStep();
      } else {
        curProgress = 0.0f;
      }
      testProgressBar->setProgressValue(curProgress);
      wm->update();
    }
#if defined(TWM_RENDER_PERF)
    beginTime = millis();
#endif
    wm->renderFrame();
#if defined(TWM_RENDER_PERF)
    accumulator += (millis() - beginTime);
    if (++paintCount >= 100) {
      TWM_LOG(TWM_DEBUG, "avg. render time, last 100 frames: %lums", accumulator / 100);
      paintCount = 0;
      accumulator = 0;
    }
#endif
  } catch (const std::exception& ex) {
    TWM_LOG(TWM_ERROR, "exception: '%s'", ex.what());
  }
}
