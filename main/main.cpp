#include <Adafruit_FT6206.h>
#include <Adafruit_CST8XX.h>
#include <Adafruit_GFX.h>
#include <Arduino_GFX_Library.h>
#include <freertos/FreeRTOS.h>
#include "sdkconfig.h"
#include <Arduino.h>

//#define TFT_720_SQUARE
//#define TFT_480_ROUND
//#define TFT_320_RECTANGLE
#define TFT_480_RECTANGLE
//#define TFT_800_RECTANGLE

#if defined(TFT_720_SQUARE)
# include <Fonts/FreeSans18pt7b.h>
# define DEFAULT_FONT &FreeSans18pt7b
# define DISPLAY_WIDTH 720
# define DISPLAY_HEIGHT 720
# define TFT_ROTATION 0
# define I2C_TOUCH_ADDR 0x48
//# define EWM_GFX_ADAFRUIT
# define EWM_GFX_ARDUINO
#elif defined(TFT_480_ROUND)
# include <Fonts/FreeSans12pt7b.h>
# define DEFAULT_FONT &FreeSans12pt7b
# define DISPLAY_WIDTH 480
# define DISPLAY_HEIGHT 480
# define TFT_ROTATION 0
# define I2C_TOUCH_ADDR 0x15
//# define EWM_GFX_ADAFRUIT
# define EWM_GFX_ARDUINO
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
# define EWM_GFX_ADAFRUIT
# define EYESPI_DISPLAY
# include <Adafruit_ILI9341.h>
/* # define EWM_GFX_ARDUINO
# include <display/Arduino_ILI9341.h> */
#elif defined(TFT_480_RECTANGLE)
# include <Fonts/FreeSans12pt7b.h>
# define DEFAULT_FONT &FreeSans12pt7b
# define DISPLAY_WIDTH 320
# define DISPLAY_HEIGHT 480
# define TFT_ROTATION 3
# define TS_MINX 0
# define TS_MINY 0
# define TS_MAXX DISPLAY_WIDTH
# define TS_MAXY DISPLAY_HEIGHT
# define I2C_TOUCH_ADDR 0x38
# define EWM_GFX_ADAFRUIT
# define EYESPI_DISPLAY
# include <Adafruit_HX8357.h>
# include <Adafruit_FT5336.h>
#elif defined(TFT_800_RECTANGLE)
# include <Fonts/FreeSans12pt7b.h>
# define DEFAULT_FONT &FreeSans12pt7b
# define DISPLAY_WIDTH 800
# define DISPLAY_HEIGHT 480
# define TFT_ROTATION 0
# define TS_MINX 0
# define TS_MINY 0
# define TS_MAXX DISPLAY_WIDTH
# define TS_MAXY DISPLAY_HEIGHT
# define EWM_GFX_ADAFRUIT
//# define EWM_GFX_ARDUINO
# define EWM_ADAFRUIT_RA8875
# include <Adafruit_RA8875.h>
#else
# error "invalid display selection"
#endif

#if DISPLAY_WIDTH != DISPLAY_HEIGHT
# define COORDINATE_MAPPING
#endif

#define TFT_SCREENSAVER_AFTER 0.5 * 60 * 1000

#include "exostra.h"
using namespace exostra;

#if defined(TFT_480_RECTANGLE)
Adafruit_FT5336 ctp;
#else
Adafruit_FT6206 focal_ctp;
Adafruit_CST8XX cst_ctp;
#endif

#if defined(ARDUINO_PROS3) || defined(ARDUINO_FEATHERS3)
# define S3
#endif

#if defined(EYESPI_DISPLAY) && !defined(S3)
# error "only the UM ProS3 and Feather S3 are configured for use with this display"
#endif

#define PIN_SCK  SCK
#define PIN_MOSI MOSI
#define PIN_MISO MISO

#if defined(S3) // Unexpected Maker ProS3 or FeatherS3.
# if defined(ARDUINO_PROS3)
#  define PIN_DC   GPIO_NUM_13
#  define PIN_CS   GPIO_NUM_12
#  define PIN_RST  GPIO_NUM_14
# elif defined(ARDUINO_FEATHERS3)
#  define PIN_CS   GPIO_NUM_33
#  define PIN_RST  GPIO_NUM_38
#  define PIN_INT  GPIO_NUM_1
#  define PIN_LITE GPIO_NUM_3
# endif
#else
# if defined(ARDUINO_ADAFRUIT_FEATHER_ESP32S2_REVTFT)
#  define PIN_CS   GPIO_NUM_5
#  define PIN_RST  GPIO_NUM_6
#  define PIN_INT  GPIO_NUM_9
# endif
#endif

#if defined(S3) || defined(ARDUINO_ADAFRUIT_FEATHER_ESP32S2_REVTFT)
# if defined(EWM_GFX_ADAFRUIT)
#  if defined(TFT_320_RECTANGLE)
auto display = std::make_shared<Adafruit_ILI9341>(PIN_CS, PIN_DC);
#  elif defined(TFT_480_RECTANGLE)
auto display = std::make_shared<Adafruit_HX8357>(PIN_CS, PIN_DC);
#  elif defined(EWM_ADAFRUIT_RA8875)
auto display = std::make_shared<Adafruit_RA8875>(PIN_CS, PIN_RST);
#  endif
# elif defined(EWM_GFX_ARDUINO)
Arduino_ESP32SPI bus(PIN_DC, PIN_CS, PIN_SCK, PIN_MOSI, PIN_MISO);
auto display = std::make_shared<Arduino_ILI9341>(&bus);
# endif
auto wm = createWindowManager(
    display,
    std::make_shared<DefaultTheme>(),
    DEFAULT_FONT
);
#else // !S3
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
    1 /* hsync_polarity */, 50 /* hsync_front_porch */, 2 /* hsync_pulse_width */, 44 /* hsync_back_porch */,
    1 /* vsync_polarity */, 16 /* vsync_front_porch */, 2 /* vsync_pulse_width */, 18 /* vsync_back_porch */
    //1 /* hync_polarity */, 46 /* hsync_front_porch */, 2 /* hsync_pulse_width */, 44 /* hsync_back_porch */,
    //1 /* vsync_polarity */, 50 /* vsync_front_porch */, 16 /* vsync_pulse_width */, 16 /* vsync_back_porch */
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
auto wm = createWindowManager(
    display,
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
#if defined(S3)
  /*ums3.setPixelPower(true); // assume pixel could be off.
  ums3.setPixelBrightness(255); // maximum brightness.*/
  pinMode(13, OUTPUT);
  while (true) {
    EWM_LOG_E("!! fatal error !!");
    //ums3.setPixelColor(0xff, 0x00, 0x00); // pure red.
    digitalWrite(13, LOW);
    delay(1000);
    //ums3.setPixelColor(0x00, 0x00, 0x00); // black (off).
    digitalWrite(13, HIGH);
    delay(1000);
  }
#else
  /// TODO: blink an LED or something.
  EWM_LOG_E("fatal error");
  while (true);
#endif
}

std::shared_ptr<TestProgressBar> testProgressBar;
std::shared_ptr<TestOKPrompt> okPrompt;

bool isFocalTouch = false;

void setup(void)
{
  delay(500);
  EWM_LOG_D("initializing");

#if defined(EYESPI_DISPLAY)
  bool touchInitialized = false;
#endif

  auto logMemoryValue = [](const char* name, uint32_t val)
  {
    EWM_LOG_D("%s: %.2f KiB", name, val / 1024.0f);
  };

  logMemoryValue("Total RAM", ESP.getHeapSize());
  logMemoryValue("Free RAM", ESP.getFreeHeap());
  logMemoryValue("Total PSRAM", ESP.getPsramSize());
  logMemoryValue("Free PSRAM", ESP.getFreePsram());
  logMemoryValue("Total flash", ESP.getFlashChipSize());

#if defined(EYESPI_DISPLAY) && defined(EWM_GFX_ARDUINO)
  bus.begin();
#endif

#if defined(EWM_ADAFRUIT_RA8875)
  if (!wm->begin(TFT_ROTATION, RA8875_800x480)) {
#elif defined(EWM_GFX_ADAFRUIT)
  if (!wm->begin(TFT_ROTATION, 0)) {
#else
  if (!wm->begin(TFT_ROTATION)) {
#endif
    EWM_LOG_E("WindowManager: error");
    on_fatal_error();
  }
  EWM_LOG_I("WindowManager: OK");
#if defined(EWM_ADAFRUIT_RA8875)
  display->displayOn(true);
  display->GPIOX(true);
  display->PWM1config(true, RA8875_PWM_CLK_DIV1024);
  display->PWM1out(255);
  pinMode(PIN_INT, INPUT);
  digitalWrite(PIN_INT, HIGH);
  display->touchEnable(true);
#endif
  wm->enableScreensaver(TFT_SCREENSAVER_AFTER);
#if defined(EWM_ADAFRUIT_RA8875)
  pinMode(PIN_LITE, OUTPUT);
  digitalWrite(PIN_LITE, HIGH);
#else
# if !defined(EYESPI_DISPLAY)
  expander->pinMode(PIN_NS::PCA_TFT_BACKLIGHT, OUTPUT);
  expander->digitalWrite(PIN_NS::PCA_TFT_BACKLIGHT, HIGH);
# endif
  display->fillScreen(0xb5be);
# if !defined(TFT_480_RECTANGLE) && !defined(TFT_800_RECTANGLE)
  if (!focal_ctp.begin(0, &Wire, I2C_TOUCH_ADDR)) {
    EWM_LOG_E("FT6206: error at 0x%x", I2C_TOUCH_ADDR);
    if (!cst_ctp.begin(&Wire, I2C_TOUCH_ADDR)) {
      EWM_LOG_E("CST8XX: error at 0x%x", I2C_TOUCH_ADDR);
    } else {
      touchInitialized = true;
      isFocalTouch = false;
      EWM_LOG_I("CST8XX: OK");
    }
  } else {
    touchInitialized = isFocalTouch = true;
    EWM_LOG_I("FT6206: OK");
  }
# elif defined(TFT_480_RECTANGLE)
  touchInitialized = ctp.begin(I2C_TOUCH_ADDR);
  if (!touchInitialized) {
    EWM_LOG_E("FT5336: error at 0x%x", I2C_TOUCH_ADDR);
  } else {
    EWM_LOG_I("FT5336: OK");
  }
# endif
  if (!touchInitialized) {
    on_fatal_error();
  }
#endif

  WindowID id     = 1;
  auto theme      = wm->getTheme();
  auto xPadding   = theme->getMetric(MetricID::XPadding).getExtent();
  auto yPadding   = theme->getMetric(MetricID::YPadding).getExtent();
  auto defaultWin = wm->createWindow<DefaultWindow>(
    nullptr,
    id++,
    Style::Visible | Style::TopLevel,
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
    Style::Button | Style::Child | Style::Visible | Style::AutoSize,
    defaultWin->getRect().left + xPadding,
    defaultWin->getRect().top + yPadding,
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
    Style::Label | Style::Child | Style::Visible,
    button1->getRect().right + xPadding,
    button1->getRect().top,
    button1->getRect().width(),
    theme->getMetric(MetricID::DefButtonCY).getExtent(),
    "Label"
  );
  if (!label1) {
    on_fatal_error();
  }

  testProgressBar = wm->createProgressBar<TestProgressBar>(
    defaultWin,
    id++,
    Style::Progress | Style::Child | Style::Visible,
    defaultWin->getRect().left + xPadding,
    button1->getRect().bottom + yPadding,
    defaultWin->getRect().width() - (xPadding * 2),
    theme->getMetric(MetricID::DefProgressHeight).getExtent(),
    ProgressStyle::Normal
  );
  if (!testProgressBar) {
    on_fatal_error();
  }

  auto testCheckbox = wm->createWindow<TestCheckbox>(
    defaultWin,
    id++,
    Style::CheckBox | Style::Child | Style::Visible,
    defaultWin->getRect().left + xPadding,
    testProgressBar->getRect().bottom + yPadding,
    theme->getScaledValue(130),
    theme->getMetric(MetricID::DefCheckBoxHeight).getExtent(),
    "CheckBox"
  );
  if (!testCheckbox) {
    on_fatal_error();
  }

  okPrompt = wm->createPrompt<TestOKPrompt>(
    nullptr,
    id++,
    Style::Prompt,
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
    Style::Prompt,
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
  EWM_LOG_I("setup completed");
}

#if defined(COORDINATE_MAPPING)
# if TFT_ROTATION != 3 && TFT_ROTATION != 0
#  error "only rotation orientations 3 and 0 are implemented"
# endif
long mapXCoord(Coord x) noexcept
{
# if TFT_ROTATION == 0
  return x;
# elif TFT_ROTATION == 3
  return map(x, TS_MINX, TS_MAXX, TS_MAXX, TS_MINX);
# endif
}
long mapYCoord(Coord y) noexcept
{
# if TFT_ROTATION == 0
  return y;
# elif TFT_ROTATION == 3
  return map(y, TS_MINY, TS_MAXY, TS_MAXY, TS_MINY);
# endif
}
std::pair<Coord, Coord> swapCoords(Coord x, Coord y)
{
# if TFT_ROTATION == 3
  auto tmp = y;
  y = x;
  x = wm->getDisplayWidth() - tmp;
# endif
  return std::make_pair(x, y);
}
#endif

float curProgress = 0.0f;
float progressStep = wm->getTheme()->getMetric(MetricID::ProgressMarqueeStep).getFloat();
uint32_t lastProgress = 0U;

void loop()
{
#if defined(EWM_ADAFRUIT_RA8875)
  static constexpr float xScale = 1024.0f / DISPLAY_WIDTH;
  static constexpr float yScale = 1024.0f / DISPLAY_HEIGHT;
  if (!digitalRead(PIN_INT)) {
    if (display->touched()) {
      uint16_t x, y;
      if (display->touchRead(&x, &y)) {
        wm->hitTest(static_cast<Coord>(x / xScale), static_cast<Coord>(y / yScale));
      }
    }
  }
#else
# if !defined(TFT_480_RECTANGLE)
  if (isFocalTouch && focal_ctp.touched()) {
    TS_Point pt = focal_ctp.getPoint();
#  if defined(COORDINATE_MAPPING)
    const auto tmp = swapCoords(mapXCoord(pt.x), mapYCoord(pt.y));
    pt.x = tmp.first;
    pt.y = tmp.second;
#  endif
    wm->hitTest(pt.x, pt.y);
  } else if (!isFocalTouch && cst_ctp.touched()) {
    CST_TS_Point pt = cst_ctp.getPoint();
#  if defined(COORDINATE_MAPPING)
    const auto tmp = swapCoords(mapXCoord(pt.x), mapYCoord(pt.y));
    pt.x = tmp.first;
    pt.y = tmp.second;
#  endif
    wm->hitTest(pt.x, pt.y);
  }
# elif defined(TFT_480_RECTANGLE)
  if (ctp.touched() > 0) {
    FT_TS_Point pt = ctp.getPoint();
    auto tmp = pt.y;
#  if defined(COORDINATE_MAPPING)
    pt.y = mapXCoord(wm->getDisplayHeight() - pt.x);
    pt.x = mapYCoord(tmp);
#  endif
    if (pt.x >= 0 && pt.y >= 0) {
      wm->hitTest(pt.x, pt.y);
    }
  }
# endif
#endif
  if (millis() - lastProgress > 500U) {
    lastProgress = millis();
    if (curProgress < 100.0f) {
      curProgress += progressStep;
    } else {
      curProgress = 0.0f;
    }
    testProgressBar->setProgressValue(curProgress);
  }
  wm->render();
}
