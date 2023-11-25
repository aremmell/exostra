/***
 * TFT capacative touch on ProS3. Pins:
 * sda 8
 * scl 9
 * tcs 12
 * dc 13
 * rst 14
 */

#include <twm.hh>
#include <SPI.h>
#include <Wire.h>
#include <Adafruit_ILI9341.h>
#include <Adafruit_FT6206.h>
#include <aremmell_um.h>
#include <UMS3.h>

// The display also uses hardware SPI, plus these pins:
#define TFT_CS 12
#define TFT_DC 13

// If no touches are registered in this time, paint the screen
// black as a pseudo-screensaver. In the future, save what was on
// the screen and restore it after.
#define TFT_TOUCH_TIMEOUT 60000

// Display size
#define TFT_WIDTH 240
#define TFT_HEIGHT 320

// Calibration data
#define TS_MINX 0
#define TS_MINY 0
#define TS_MAXX 240
#define TS_MAXY 320

using namespace aremmell;
using namespace twm;

// The FT6206 uses hardware I2C (SCL/SDA)
Adafruit_FT6206 ctp;
Adafruit_ILI9341 display(TFT_CS, TFT_DC);
UMS3 ums3;

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

void setup(void)
{
  Serial.begin(115200);
  while (!Serial) {
    /// TODO: If you want to run without a serial cable, exit this loop
    delay(10);
  }

  delay(500);

  ums3.begin();

  if (!ctp.begin(40, &Wire)) {
    Serial.println("FT6206: error!");
    on_fatal_error(ums3);
  }

  Serial.println("FT6206: OK");

  display.begin();
  display.setRotation(3);
  display.setCursor(0, 0);

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
    on_fatal_error(ums3);
  }

  auto button1 = wm->createWindow<EveryDayNormalButton>(defaultWin, 3,
    STY_CHILD | STY_VISIBLE | STY_AUTOSIZE | STY_BUTTON, 40, 50, 0, 0,
    "pres me");
  if (!button1) {
    on_fatal_error(ums3);
  }

  auto labelX = defaultWin->getRect().width() - (130 - xPadding);
  auto label1 = wm->createWindow<TestLabel>(defaultWin, 4, STY_CHILD | STY_VISIBLE | STY_LABEL,
    labelX, 50, 130 - xPadding, 30, "A static label");
  if (!label1) {
    on_fatal_error(ums3);
  }
  button1->setLabel(label1);

  auto yPadding = wm->getTheme()->getWindowYPadding();
  testProgressBar = wm->createProgressBar<TestProgressBar>(defaultWin, 5,
    STY_CHILD | STY_VISIBLE | STY_PROGBAR, xPadding * 2, 90 + yPadding,
    defaultWin->getRect().width() - (xPadding * 2), 30, PBR_INDETERMINATE);
  if (!testProgressBar) {
    on_fatal_error(ums3);
  }

  okPrompt = wm->createPrompt<TestOKPrompt>(
    nullptr,
    6,
    STY_PROMPT,
    "You did a thing, and now this is on your screen.",
    {{100, "OK"}},
    [](WindowID id)
    {
      TWM_LOG(TWM_DEBUG, "dismiss OK prompt");
    }
  );
  if (!okPrompt) {
    on_fatal_error(ums3);
  }

  yesNoPromptWnd = wm->createPrompt<TestYesNoPrompt>(
    nullptr,
    7,
    STY_PROMPT,
    "This is a test prompt. Please choose an option.",
    {{100, "Yes"}, {101, "No"}},
    [&](WindowID id)
    {
      TWM_LOG(TWM_DEBUG, "prompt button chosen: %hhu", id);
      std::string prompt = "You tapped the ";
      prompt += id == 100 ? "Yes" : "No";
      prompt += " button.";
      okPrompt->setText(prompt);
      okPrompt->show();
    }
  );
  if (!yesNoPromptWnd) {
    on_fatal_error(ums3);
  }
  button1->setPrompt(yesNoPromptWnd);
}

float curProgress = 0.0f;

u_long lastTouch = 0UL;
bool screensaverOn = false;

void loop()
{
  if (ctp.touched()) {
    lastTouch = millis();
    if (screensaverOn) {
      screensaverOn = false;
    }

    TS_Point pt = ctp.getPoint();
    pt.x = map(pt.x, TS_MINX, TS_MAXX, TS_MAXX, TS_MINX);
    pt.y = map(pt.y, TS_MINY, TS_MAXY, TS_MAXY, TS_MINY);

    long tmp = pt.y;
    pt.y = pt.x;
    pt.x = wm->getGfx()->width() - tmp;

    wm->hitTest(pt.x, pt.y);
  } else {
    if (!screensaverOn && millis() - lastTouch > TFT_TOUCH_TIMEOUT) {
TODO_refactor:
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

  display.drawRGBBitmap(0, 0, wm->getGfx()->getBuffer(), wm->getGfx()->width(), wm->getGfx()->height());
}
