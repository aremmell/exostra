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
#include <Adafruit_ILI9341.h>
#include <Wire.h>
#include <Adafruit_FT6206.h>
#include <UMS3.h>
#include <aremmell_um.h>

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
UMS3 ums3;

using TwmGfxCtx = GfxContext<Adafruit_ILI9341>;
auto tft = TwmGfxCtx::init(TFT_CS, TFT_DC);

static inline
void draw_initial_screen()
{
  tft->fillScreen(ILI9341_BLACK);
  tft->println("Go ahead, touch that smoke wagon.");
}

void setup(void)
{
  Serial.begin(115200);
  while (!Serial) {
    /// TODO: If you want to run without a serial cable, exit this loop
    delay(10);
  }

  TWM_ASSERT(tft == TwmGfxCtx::getInstance());

  ums3.begin();
  tft->begin();

  if (!ctp.begin(40, &Wire)) {
    Serial.println("FT6206: error!");
    on_fatal_error(ums3);
  }

  Serial.println("FT6206: OK");

  tft->setRotation(3);
  tft->setCursor(0, 0);

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
  if (ctp.touched()) {
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
    pt.x = tft->width() - tmp;

    tft->drawCircle(pt.x, pt.y, 5, ILI9341_CYAN);
  } else {
    if (!screensaver_on && millis() - last_touch > TFT_TOUCH_TIMEOUT) {
      tft->fillScreen(ILI9341_BLACK);
      screensaver_on = true;
    }
  }
}
