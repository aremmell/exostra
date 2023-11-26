#include <Arduino.h>
#include <SPI.h>
#include <Wire.h>
#include <Arduino_GFX_Library.h>
#include <twm.hh>
#include <Adafruit_FT6206.h>
#include <Adafruit_CST8XX.h>
#if defined(ARDUINO_PROS3) && !defined(QUALIA)
/***
 * TFT capacative touch on ProS3. Pins:
 * sda 8
 * scl 9
 * tcs 12
 * dc 13
 * rst 14
 *
 * The display also uses hardware SPI, plus these pins:
 */
# define TFT_CS 12
# define TFT_DC 13
/*
 * Display size
 */
# define TFT_WIDTH 240
# define TFT_HEIGHT 320
# define I2C_TOUCH_ADDR 0x38
# include <Adafruit_ILI9341.h>
# include <UMS3.h>
# include <aremmell_um.h>
using namespace aremmell;
// Calibration data
# define TS_MINX 0
# define TS_MINY 0
# define TS_MAXX 240
# define TS_MAXY 320
# else // Implied Qualia RGB666 for now.
# if !defined(ARDUINO)
#  include <esp32_qualia.h>
#  define PIN_NS qualia
# else
#  define PIN_NS
# endif
# define TFT_WIDTH 720
# define TFT_HEIGHT 720
# define I2C_TOUCH_ADDR 0x48//0x15 0x3f, 0x38
#endif

// If no touches are registered in this time, paint the screen
// black as a pseudo-screensaver. In the future, save what was on
// the screen and restore it after.
#define TFT_TOUCH_TIMEOUT 60000

using namespace twm;

// The FT6206 uses hardware I2C (SCL/SDA)
Adafruit_FT6206 focal_ctp;
Adafruit_CST8XX cst_ctp;

#if defined(ARDUINO_PROS3) && !defined(QUALIA)
Adafruit_ILI9341 display(TFT_CS, TFT_DC);
UMS3 ums3;
#else
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
  //    ,1, 30000000
    );

Arduino_RGB_Display *display = new Arduino_RGB_Display(
  // 4.0" 720x720 square display
  720 /* width */, 720 /* height */, rgbpanel, 0 /* rotation */, true /* auto_flush */,
  expander, GFX_NOT_DEFINED /* RST */, NULL, 0);
  // 4.0" 720x720 round display
  //    720 /* width */, 720 /* height */, rgbpanel, 0 /* rotation */, true /* auto_flush */,
  //    expander, GFX_NOT_DEFINED /* RST */, hd40015c40_init_operations, sizeof(hd40015c40_init_operations));
  // needs also the rgbpanel to have these pulse/sync values:
  //    1 /* hync_polarity */, 46 /* hsync_front_porch */, 2 /* hsync_pulse_width */, 44 /* hsync_back_porch */,
  //    1 /* vsync_polarity */, 50 /* vsync_front_porch */, 16 /* vsync_pulse_width */, 16 /* vsync_back_porch */
#endif

auto wm = std::make_shared<TWM>(
  std::make_shared<GFXcanvas16>(TFT_HEIGHT, TFT_WIDTH),
  std::make_shared<DefaultTheme>()
);

class EveryDayNormalButton : public Button
{
public:
  using Button::Button;
  EveryDayNormalButton() = default;
  virtual ~EveryDayNormalButton() = default;

  void setLabel(const WindowPtr& label) { _label = label; }
  void setPrompt(const WindowPtr& prompt) { _prompt = prompt; }

  void onTapped() override
  {
    Button::onTapped();
    if (_label) {
      _label->setText("Tapped!");
    }
    if (_prompt) {
      _prompt->show();
    }
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

void on_fatal_error()
{
#if defined(ARDUINO_PROS3) && !defined(QUALIA)
  aremmell::on_fatal_error(ums3);
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
  Serial.begin(115200);
  while (!Serial) {
    /// TODO: If you want to run without a serial cable, exit this loop
    delay(10);
  }

  delay(500);

#if defined(ARDUINO_PROS3) && !defined(QUALIA)
  ums3.begin();
  display.begin();
  display.setRotation(3);
  display.setCursor(0, 0);
#else
#ifdef GFX_EXTRA_PRE_INIT
  GFX_EXTRA_PRE_INIT();
#endif
  Wire.setClock(1000000);
  if (!display->begin()) {
    Serial.println("RGBDisplay: error!");
    on_fatal_error();
  }
  Serial.println("RGBDisplay: OK");
  wm->getTheme()->drawDesktopBackground();
  expander->pinMode(PIN_NS::PCA_TFT_BACKLIGHT, OUTPUT);
  expander->digitalWrite(PIN_NS::PCA_TFT_BACKLIGHT, HIGH);
#endif

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

  auto x = scaledValue(40);
  auto y = scaledValue(50);
  auto button1 = wm->createWindow<EveryDayNormalButton>(defaultWin, 3,
    STY_CHILD | STY_VISIBLE | STY_AUTOSIZE | STY_BUTTON, x, y, 0, 0,
    "pres me");
  if (!button1) {
    on_fatal_error();
  }

  auto cx = scaledValue(70);
  auto cy = scaledValue(30);
  x = defaultWin->getRect().left + defaultWin->getRect().width() - (cx - xPadding);
  y = scaledValue(50);
  auto label1 = wm->createWindow<TestLabel>(defaultWin, 4, STY_CHILD | STY_VISIBLE | STY_LABEL,
    x, y, cx - xPadding, cy, "A static label");
  if (!label1) {
    on_fatal_error();
  }
  button1->setLabel(label1);

  auto yPadding = wm->getTheme()->getWindowYPadding();

  cx = scaledValue(90);
  cy = scaledValue(30);
  testProgressBar = wm->createProgressBar<TestProgressBar>(defaultWin, 5,
    STY_CHILD | STY_VISIBLE | STY_PROGBAR, xPadding * 2, button1->getRect().bottom + yPadding,
    defaultWin->getRect().width() - (xPadding * 2), cy, PBR_INDETERMINATE);
  if (!testProgressBar) {
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
}

float curProgress = 0.0f;

u_long lastTouch = 0UL;
bool screensaverOn = false;

void loop()
{
  if (isFocalTouch && focal_ctp.touched()) {
    lastTouch = millis();
    if (screensaverOn) {
      screensaverOn = false;
    }
    TS_Point pt = focal_ctp.getPoint();
#if defined(ARDUINO_PROS3) && !defined(QUALIA)
    // Rotated rectangular display.
    pt.x = map(pt.x, TS_MINX, TS_MAXX, TS_MAXX, TS_MINX);
    pt.y = map(pt.y, TS_MINY, TS_MAXY, TS_MAXY, TS_MINY);
    long tmp = pt.y;
    pt.y = pt.x;
    pt.x = wm->getGfx()->width() - tmp;
#endif
    wm->hitTest(pt.x, pt.y);
  } else if (!isFocalTouch && cst_ctp.touched()) {
    lastTouch = millis();
    if (screensaverOn) {
      screensaverOn = false;
    }
    CST_TS_Point pt = cst_ctp.getPoint();
#if defined(ARDUINO_PROS3) && !defined(QUALIA)
    // Rotated rectangular display.
    pt.x = map(pt.x, TS_MINX, TS_MAXX, TS_MAXX, TS_MINX);
    pt.y = map(pt.y, TS_MINY, TS_MAXY, TS_MAXY, TS_MINY);
    long tmp = pt.y;
    pt.y = pt.x;
    pt.x = wm->getGfx()->width() - tmp;
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
      curProgress += 1.0f;
    } else {
      curProgress = 0.0f;
    }
    testProgressBar->setProgressValue(curProgress);
    wm->update();
  }
#if defined(ARDUINO_PROS3) && !defined(QUALIA)
  display.drawRGBBitmap(
    0,
    0,
    wm->getGfx()->getBuffer(),
    wm->getGfx()->width(),
    wm->getGfx()->height()
  );
#else
  display->draw16bitRGBBitmap(
    0,
    0,
    wm->getGfx()->getBuffer(),
    wm->getScreenWidth(),
    wm->getScreenHeight()
  );
#endif
}
