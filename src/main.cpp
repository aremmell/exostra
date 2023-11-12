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
#include <Fonts/FreeSans12pt7b.h>
#include <aremmell_um.h>
#include <UMS3.h>

// The display also uses hardware SPI, plus these pins:
#define TFT_CS 12
#define TFT_DC 13

// If no touches are registered in this time, paint the screen
// black as a pseudo-screensaver. In the future, save what was on
// the screen and restore it after.
#define TFT_TOUCH_TIMEOUT 10000

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

static inline
void draw_initial_screen()
{
  wm->fillScreen(ILI9341_BLACK);
  //wm->setTextSize(2);
  //wm->println("Go ahead, touch that smoke wagon.");
  display.drawRGBBitmap(0, 0, wm->getBuffer(), wm->width(), wm->height());
}

class DesktopWnd : public Window
{
  public:
    using Window::Window;
    DesktopWnd() = default;
    virtual ~DesktopWnd() = default;

protected:
    void onDraw(void* param1, void* param2) final
    {
      // The desktop is just a boring window that takes up the entire screen.
      auto rect = getRectangle();
      wm->fillRect(rect.left, rect.top, rect.width(), rect.height(), 0xb59a);
      wm->setTextSize(3);
      wm->setTextColor(0x0000);
      wm->setCursor(0, 0);
      wm->print("sups?");
    }
};

void setup(void)
{
  Serial.begin(115200);
  while (!Serial) {
    /// TODO: If you want to run without a serial cable, exit this loop
    delay(10);
  }

  delay(500);

  auto desktop = wm->createWindow<DesktopWnd>(nullptr, Style::Visible, 0, 0, TFT_HEIGHT, TFT_WIDTH);
  if (!desktop) {
    on_fatal_error(ums3);
  }

  /*auto window1 = wm->createWindow(desktop, Style::Child | Style::Visible,
    (desktop->getRectangle().width() / 2) - 100,
    (desktop->getRectangle().height() / 2) - 50, 200, 100);
  if (!window1) {
    on_fatal_error(ums3);
  }

  window1->registerHandler(MSG_DRAW, [&](void* payload)
  {
    auto rect = window1->getRectangle();

    wm->fillRect(rect.left, rect.top + 20, rect.width(), rect.height(), 0xb59a);
    rect.shrink(1);
    wm->fillRect(rect.left, rect.top, rect.width(), 20, 0x5a7b);
    wm->drawRect(rect.left, rect.right, rect.width(), rect.height(), 0x5a7b);
    wm->setTextSize(1);
    wm->setCursor(rect.left + 5, rect.top);
    wm->setTextColor(0x5a7b);
    wm->print("my first ghetto window");

    wm->setCursor(rect.left + 5, rect.top + 25);
    wm->println("This is where a client area will one day exist");
  });*/

  ums3.begin();

  if (!ctp.begin(40, &Wire)) {
    Serial.println("FT6206: error!");
    on_fatal_error(ums3);
  }

  Serial.println("FT6206: OK");

  //wm->begin();
  display.begin();
  display.setRotation(3);
  display.setCursor(0, 0);

  draw_initial_screen();
}

u_long last_touch = 0UL;
bool screensaver_on = false;

static inline
void on_screensaver_gone()
{
  draw_initial_screen();
}

void loop()
{
  /*if (ctp.touched()) {
    last_touch = millis();
    if (screensaver_on) {
      screensaver_on = false;
      draw_initial_screen();
    }
    TS_Point pt = ctp.getPoint();

    pt.x = map(pt.x, TS_MINX, TS_MAXX, TS_MAXX, TS_MINX);
    pt.y = map(pt.y, TS_MINY, TS_MAXY, TS_MAXY, TS_MINY);

    long tmp = pt.y;
    pt.y = pt.x;
    pt.x = wm->width() - tmp;

    wm->drawCircle(pt.x, pt.y, 5, ILI9341_CYAN);
  } else {
    if (!screensaver_on && millis() - last_touch > TFT_TOUCH_TIMEOUT) {
      wm->fillScreen(ILI9341_BLACK);
      screensaver_on = true;
    }
  }*/
  wm->update();
  display.drawRGBBitmap(0, 0, wm->getBuffer(), wm->width(), wm->height());
  delay(250);
}
