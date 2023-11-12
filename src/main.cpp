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
#include <Fonts/FreeSans9pt7b.h>
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

auto wm = TWM<GFXcanvas16>::init(TFT_HEIGHT, TFT_WIDTH);

class DesktopWnd : public Window
{
  public:
    using Window::Window;
    DesktopWnd() = default;
    virtual ~DesktopWnd() = default;

protected:
    bool onDraw(void* param1, void* param2) final
    {
      auto rect = getRect();
      wm->fillRect(rect.left, rect.top, rect.width(), rect.height(), 0xb59a);
      return true;
    }
};

class Button : public Window
{
public:
  using Window::Window;
  Button() = default;
  virtual ~Button() = default;

protected:
  u_long lastTapped = 0UL;
  void onTapped()
  {
    lastTapped = millis();
    TWM_LOG(TWM_DEBUG, "window %hhu was tapped!", getID());

    wm->destroyWindow(3);
  }

  bool onDraw(void* param1, void* param2) final
  {
    bool tappedRecently = millis() - lastTapped < 500;
    auto rect = getRect();
    wm->fillRoundRect(50, 50, 85, 75, 5, tappedRecently ? ILI9341_BLUE : ILI9341_DARKGREY);
    wm->setTextColor(0xFFFF);
    wm->setCursor(60, 87);
    wm->print("tap me!");
    return true;
  }

  bool onInput(void* param1, void* param2) final
  {
    InputParams* ip = static_cast<InputParams*>(param1);
    if (ip != nullptr) {
      switch(ip->type) {
        case INPUT_TAP: onTapped(); return true;
        default: break;
      }
    }
    return false;
  }
};

class DefaultWindow : public Window
{
public:
  using Window::Window;
  DefaultWindow() = default;
  virtual ~DefaultWindow() = default;
};

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

  wm->fillScreen(ILI9341_BLACK);
  wm->setFont(&FreeSans9pt7b);

  auto desktop = wm->createWindow<DesktopWnd>(nullptr, STY_VISIBLE, 0, 0, TFT_HEIGHT, TFT_WIDTH);
  if (!desktop) {
    on_fatal_error(ums3);
  }

  auto button1 = wm->createWindow<Button>(desktop, STY_CHILD | STY_VISIBLE, 50, 50, 85, 75);
  if (!button1) {
    on_fatal_error(ums3);
  }

  auto defaultWin = wm->createWindow<DefaultWindow>(desktop, STY_CHILD | STY_VISIBLE,
    (TFT_HEIGHT / 2), 50, 85, 85);
  if (!defaultWin) {
    on_fatal_error(ums3);
  }
}

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
    pt.x = wm->width() - tmp;

    wm->hitTest(pt.x, pt.y);
  } else {
    if (!screensaverOn && millis() - lastTouch > TFT_TOUCH_TIMEOUT) {
      wm->fillScreen(0x0000);
      screensaverOn = true;
    }
  }

  if (!screensaverOn) {
    wm->update();
  }

  display.drawRGBBitmap(0, 0, wm->getBuffer(), wm->width(), wm->height());
  delay(100);
}
