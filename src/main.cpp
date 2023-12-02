#include <Arduino.h>
#include <SPI.h>
#include <Wire.h>
#include <Adafruit_FT6206.h>
#include <Adafruit_CST8XX.h>
#include <Adafruit_GFX.h>
#include <Arduino_GFX_Library.h>

#define TFT_720_SQUARE
//#define TFT_480_ROUND
//#define TFT_320_RECTANGLE

#if defined(TFT_720_SQUARE)
# include <Fonts/FreeSans18pt7b.h>
# define DEFAULT_FONT &FreeSans18pt7b
# define DISPLAY_WIDTH 720
# define DISPLAY_HEIGHT 720
# define TFT_ROTATION 0
# define I2C_TOUCH_ADDR 0x48
//# define TWM_GFX_ADAFRUIT
# define TWM_GFX_ARDUINO
#elif defined(TFT_480_ROUND)
# include <Fonts/FreeSans12pt7b.h>
# define DEFAULT_FONT &FreeSans12pt7b
# define DISPLAY_WIDTH 480
# define DISPLAY_HEIGHT 480
# define TFT_ROTATION 0
# define I2C_TOUCH_ADDR 0x15
//# define TWM_GFX_ADAFRUIT
# define TWM_GFX_ARDUINO
#elif defined(TFT_320_RECTANGLE)
# include <Fonts/FreeSans9pt7b.h>
# define DEFAULT_FONT &FreeSans9pt7b
# define DISPLAY_WIDTH 240
# define DISPLAY_HEIGHT 320
# define TFT_ROTATION 3
# define TS_MINX 0
# define TS_MINY 0
# define TS_MAXX DISPLAY_WIDTH
# define TS_MAXY DISPLAY_HEIGHT
# define I2C_TOUCH_ADDR 0x38
# define TWM_GFX_ADAFRUIT
# include <Adafruit_ILI9341.h>
/* # define TWM_GFX_ARDUINO
# include <display/Arduino_ILI9341.h> */
#else
# error "invalid display selection"
#endif

#define TFT_SCREENSAVER_AFTER 5 * 60 * 1000

#include "twm.h"
using namespace thumby;

Adafruit_FT6206 focal_ctp;
Adafruit_CST8XX cst_ctp;

#if defined(ARDUINO_PROS3)
// Unexpected Maker ProS3.
# include <UMS3.h>
UMS3 ums3;
# define PIN_DC   13
# define PIN_CS   12
# define PIN_SCK  36
# define PIN_MOSI 35
# define PIN_MISO 37
#endif

#if defined(TFT_320_RECTANGLE)
# if !defined(ARDUINO_PROS3)
#  error "only the UM ProS3 is configured for use with this display"
# endif

#if defined(TWM_GFX_ADAFRUIT)
auto display = std::make_shared<Adafruit_ILI9341>(PIN_CS, PIN_DC);
auto context = std::make_shared<GfxContext>(DISPLAY_HEIGHT, DISPLAY_WIDTH);
#elif defined(TWM_GFX_ARDUINO)
Arduino_ESP32SPI bus(PIN_DC, PIN_CS, PIN_SCK, PIN_MOSI, PIN_MISO);
auto display = std::make_shared<Arduino_ILI9341>(&bus);
auto context = std::make_shared<GfxContext>(DISPLAY_HEIGHT, DISPLAY_WIDTH, display.get());
#endif

auto wm = createWindowManager(
    display,
    context,
    std::make_shared<DefaultTheme>(),
    DEFAULT_FONT
);
#else
// Implied Qualia RGB666 for now.
# if defined(PLATFORMIO) /// TODO: detect qualia, hopefully remove hack
#  include <esp32_qualia.h>
#  define PIN_NS qualia
# else
#  define PIN_NS
# endif
auto expander = new Arduino_XCA9554SWSPI(
    PIN_NS::PCA_TFT_RESET, PIN_NS::PCA_TFT_CS, PIN_NS::PCA_TFT_SCK, PIN_NS::PCA_TFT_MOSI, &Wire, 0x3F
);
auto rgbpanel = new Arduino_ESP32RGBPanel(
    PIN_NS::TFT_DE, PIN_NS::TFT_VSYNC, PIN_NS::TFT_HSYNC, PIN_NS::TFT_PCLK,
    PIN_NS::TFT_R1, PIN_NS::TFT_R2, PIN_NS::TFT_R3, PIN_NS::TFT_R4, PIN_NS::TFT_R5,
    PIN_NS::TFT_G0, PIN_NS::TFT_G1, PIN_NS::TFT_G2, PIN_NS::TFT_G3, PIN_NS::TFT_G4, PIN_NS::TFT_G5,
    PIN_NS::TFT_B1, PIN_NS::TFT_B2, PIN_NS::TFT_B3, PIN_NS::TFT_B4, PIN_NS::TFT_B5,
    //1 /* hsync_polarity */, 50 /* hsync_front_porch */, 2 /* hsync_pulse_width */, 44 /* hsync_back_porch */,
    //1 /* vsync_polarity */, 16 /* vsync_front_porch */, 2 /* vsync_pulse_width */, 18 /* vsync_back_porch */
    1 /* hync_polarity */, 46 /* hsync_front_porch */, 2 /* hsync_pulse_width */, 44 /* hsync_back_porch */,
    1 /* vsync_polarity */, 50 /* vsync_front_porch */, 16 /* vsync_pulse_width */, 16 /* vsync_back_porch */
);
auto display = std::make_shared<Arduino_RGB_Display>(
  DISPLAY_WIDTH, DISPLAY_HEIGHT, rgbpanel, 0, true, expander, GFX_NOT_DEFINED,
# if defined(TFT_720_SQUARE) // 4.0" 720x720 square display
  nullptr, 0
# elif defined(TFT_480_ROUND) // 2.1" 480x480 round display
  TL021WVC02_init_operations, sizeof(TL021WVC02_init_operations)
# else
#  error "invalid display selection"
# endif
);
auto context = std::make_shared<GfxContext>(DISPLAY_WIDTH, DISPLAY_HEIGHT
#if defined(TWM_GFX_ARDUINO)
  , display.get()
#endif
);
auto wm = createWindowManager(
    display,
    context,
    std::make_shared<DefaultTheme>(),
    DEFAULT_FONT
);
#endif

class TestButton : public Button
{
public:
  using Button::Button;
  TestButton() = default;
  virtual ~TestButton() = default;

  void setPrompt(const WindowPtr& prompt) { _prompt = prompt; }

  bool onTapped(Coord x, Coord y) override
  {
    Button::onTapped(x, y);
    if (_prompt) {
      _prompt->show();
    }
    return true;
  }

private:
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
  /// TODO: blink an LED or something.
  TWM_LOG(TWM_ERROR, "fatal error");
  while (true);
#endif
}

std::shared_ptr<TestProgressBar> testProgressBar;
std::shared_ptr<TestOKPrompt> okPrompt;

bool touchInitialized = false;
bool isFocalTouch = false;

void setup(void)
{
  while (!Serial) {
    /// TODO: If you want to run without a serial cable, exit this loop
    delay(10);
  }

  delay(500);
  TWM_LOG(TWM_DEBUG, "initializing");

#if defined(TFT_320_RECTANGLE)
# if defined(TWM_GFX_ARDUINO)
  bus.begin();
# endif
  ums3.begin();
#endif
  Wire.setClock(1000000);
  if (!wm->begin()) {
    TWM_LOG(TWM_ERROR, "WindowManager: error");
    on_fatal_error();
  }
  TWM_LOG(TWM_DEBUG, "WindowManager: OK");
  wm->enableScreensaver(TFT_SCREENSAVER_AFTER);
  display->setRotation(TFT_ROTATION);
  display->setCursor(0, 0);
#if !defined(TFT_320_RECTANGLE)
  expander->pinMode(PIN_NS::PCA_TFT_BACKLIGHT, OUTPUT);
  expander->digitalWrite(PIN_NS::PCA_TFT_BACKLIGHT, HIGH);
#endif
  if (!focal_ctp.begin(0, &Wire, I2C_TOUCH_ADDR)) {
    TWM_LOG(TWM_ERROR, "FT6206: error at 0x%x", I2C_TOUCH_ADDR);
    if (!cst_ctp.begin(&Wire, I2C_TOUCH_ADDR)) {
      TWM_LOG(TWM_ERROR, "CST8XX: error at 0x%x", I2C_TOUCH_ADDR);
    } else {
      touchInitialized = true;
      isFocalTouch = false;
      TWM_LOG(TWM_DEBUG, "CST8XX: OK");
    }
  } else {
    touchInitialized = isFocalTouch = true;
    TWM_LOG(TWM_DEBUG, "FT6206: OK");
  }
  if (!touchInitialized) {
    on_fatal_error();
  }

  WindowID id = 1;
  auto theme = wm->getTheme();
  auto xPadding = theme->getXPadding();
  auto defaultWin = wm->createWindow<DefaultWindow>(
    nullptr,
    id++,
    STY_VISIBLE | STY_TOPLEVEL,
    /* 0, 0, 0, 0 */
    xPadding,
    xPadding,
    wm->getDisplayWidth() - (xPadding * 2),
    wm->getDisplayHeight() - (xPadding * 2)
  );
  if (!defaultWin) {
    on_fatal_error();
  }

  auto button1 = wm->createWindow<TestButton>(
    defaultWin,
    id++,
    STY_BUTTON | STY_CHILD | STY_VISIBLE | STY_AUTOSIZE,
    defaultWin->getRect().left + xPadding,
    theme->getScaledValue(50),
    0,
    0,
    "Button"
  );
  if (!button1) {
    on_fatal_error();
  }

  auto label1 = wm->createWindow<TestLabel>(
    defaultWin,
    id++,
    STY_LABEL | STY_CHILD | STY_VISIBLE,
    button1->getRect().right + xPadding,
    theme->getScaledValue(50),
    button1->getRect().width(),
    theme->getDefaultButtonHeight(),
    "Label"
  );
  if (!label1) {
    on_fatal_error();
  }

  auto yPadding = wm->getTheme()->getYPadding();
  testProgressBar = wm->createProgressBar<TestProgressBar>(
    defaultWin,
    id++,
    STY_PROGBAR | STY_CHILD | STY_VISIBLE,
    defaultWin->getRect().left + xPadding,
    button1->getRect().bottom + yPadding,
    defaultWin->getRect().width() - (xPadding * 2),
    wm->getTheme()->getDefaultProgressBarHeight(),
    PBR_INDETERMINATE
  );
  if (!testProgressBar) {
    on_fatal_error();
  }

  auto testCheckbox = wm->createWindow<TestCheckbox>(
    defaultWin,
    id++,
    STY_CHECKBOX | STY_CHILD | STY_VISIBLE,
    defaultWin->getRect().left + xPadding,
    testProgressBar->getRect().bottom + yPadding,
    theme->getScaledValue(130),
    theme->getScaledValue(30),
    "CheckBox"
  );
  if (!testCheckbox) {
    on_fatal_error();
  }

  okPrompt = wm->createPrompt<TestOKPrompt>(
    nullptr,
    id++,
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

  auto yesNoPromptWnd = wm->createPrompt<TestYesNoPrompt>(
    nullptr,
    id++,
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
}

#if defined(TFT_320_RECTANGLE)
long mapXCoord(Coord x) noexcept
{
  return map(x, TS_MINX, TS_MAXX, TS_MAXX, TS_MINX);
}
long mapYCoord(Coord y) noexcept
{
  return map(y, TS_MINY, TS_MAXY, TS_MAXY, TS_MINY);
}
std::pair<Coord, Coord> swapCoords(Coord x, Coord y)
{
  auto tmp = y;
  y = x;
  x = wm->getDisplayWidth() - tmp;
  return std::make_pair(x, y);
}
#endif

float curProgress = 0.0f;
float progressStep = wm->getTheme()->getProgressBarIndeterminateStep();
uint8_t frameCounter = 0;
u_long accumulator = 0UL;

void loop()
{
  u_long before = micros();
  if (isFocalTouch && focal_ctp.touched()) {
    TS_Point pt = focal_ctp.getPoint();
#if defined(TFT_320_RECTANGLE)
    const auto tmp = swapCoords(mapXCoord(pt.x), mapYCoord(pt.y));
    pt.x = tmp.first;
    pt.y = tmp.second;
#endif
    wm->hitTest(pt.x, pt.y);
  } else if (!isFocalTouch && cst_ctp.touched()) {
    CST_TS_Point pt = cst_ctp.getPoint();
#if defined(TFT_320_RECTANGLE)
    const auto tmp = swapCoords(mapXCoord(pt.x), mapYCoord(pt.y));
    pt.x = tmp.first;
    pt.y = tmp.second;
#endif
    wm->hitTest(pt.x, pt.y);
  }
  if (curProgress < 100.0f) {
    curProgress += progressStep;
  } else {
    curProgress = 0.0f;
  }
  testProgressBar->setProgressValue(curProgress);
  wm->update();
  wm->render();
  if (++frameCounter == 100) {
    TWM_LOG(TWM_DEBUG, "avg. loop time: %luμs", accumulator / 100);
    frameCounter = 0;
    accumulator = 0UL;
  }
  accumulator += micros() - before;
  delay(1);
}
