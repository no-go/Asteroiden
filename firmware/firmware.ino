#include <SPI.h> //SPI Arduino Library
//#include <Wire.h> //I2C Arduino Library

#include <Adafruit_GFX.h>
#include "Adafruit_SSD1306.h"


#define BTN_UP       A1
#define BTN_DOWN     A0
#define BTN_RIGHT    2
#define BTN_LEFT     3
#define BTN_A        7 // Fire
#define BTN_B        4 // unused
#define BTN_SW       6 // unused

#define RANDIN       A7

// OLED (11 -> MOSI/DIN, 13 ->SCK)
#define PIN_CS     8
#define PIN_DC     9
#define PIN_RESET  10 
Adafruit_SSD1306 oled(PIN_DC, PIN_RESET, PIN_CS);

// HARDWARE I2C: A4 -> SDA, A5 -> SCL
//Adafruit_SSD1306 oled(PIN_RESET);

#define SHIPSIZE      9
#define SHIPMAXSPEED 12
#define BULLETTIME   20
#define MAXBULLETS   10
#define MAXROCKS      3

struct Propa {
  short x;
  short y;
  short angle;
  short   sig;
  short    sp;
  short  tick;
};

Propa ship;

Propa buls[MAXBULLETS];
byte buid = 0;

Propa rocks[MAXROCKS];

static const uint8_t PROGMEM rock0[] = {
B01110000,
B10001000,
B01001000,
B11110000
};

static const uint8_t PROGMEM rock1[] = {
B00111111,
B11000001,
B01000011,
B00100110,
B11000001,
B10011001,
B10100110,
B01000000
};

static const uint8_t PROGMEM rock2[] = {
B00100011,B11000000,
B01011100,B00100000,
B11000000,B00010000,
B00100000,B00011100,
B00011100,B00000100,
B00001000,B00001000,
B00011000,B00001000,
B00100000,B00110000,
B00100000,B00010000,
B00010000,B00001110,
B11000000,B00000001,
B01100000,B00001110,
B00011111,B00001000,
B00001000,B10001000,
B00000000,B01001000,
B00000000,B00110000
};

void printBullets() {
  for (int i=0; i < MAXBULLETS; ++i) {  
    if (buls[i].tick > BULLETTIME) {
      buls[i].tick = -1;
      oled.drawPixel(buls[i].x, buls[i].y, BLACK);
    }
    if (buls[i].tick > 0) {
      oled.drawPixel(buls[i].x, buls[i].y, WHITE);
      buls[i].tick++;
      buls[i].x = buls[i].x - buls[i].sig * buls[i].sp * cos(PI * buls[i].angle/360.0);
      buls[i].y = buls[i].y - buls[i].sig * buls[i].sp * sin(PI * buls[i].angle/360.0);
      
      if (buls[i].x <             0) buls[i].x += oled.width();
      if (buls[i].x >  oled.width()) buls[i].x -= oled.width();
      if (buls[i].y <             0) buls[i].y += oled.height();
      if (buls[i].y > oled.height()) buls[i].y -= oled.height();
    }
  }
}

void printRocks() {
  for (int i=0; i < MAXROCKS; ++i) {  
    //oled.drawBitmap(14,  5, rock0, 8, 4, WHITE);  
    //oled.drawBitmap(54, 54, rock1, 8, 8, WHITE);    
    oled.drawBitmap(rocks[i].x, rocks[i].y, rock2, 16,16, WHITE);

    rocks[i].x = rocks[i].x + rocks[i].sp * cos(PI * rocks[i].angle/360.0);
    rocks[i].y = rocks[i].y + rocks[i].sp * sin(PI * rocks[i].angle/360.0);
    
    if (rocks[i].x <             0) rocks[i].x += oled.width();
    if (rocks[i].x >  oled.width()) rocks[i].x -= oled.width();
    if (rocks[i].y <             0) rocks[i].y += oled.height();
    if (rocks[i].y > oled.height()) rocks[i].y -= oled.height();

  }
}

void printShip() {
  ship.x = ship.x - ship.sig * ship.sp * cos(PI * ship.angle/360.0);
  ship.y = ship.y - ship.sig * ship.sp * sin(PI * ship.angle/360.0);

  if (ship.x <             0) ship.x += oled.width();
  if (ship.x >  oled.width()) ship.x -= oled.width();
  if (ship.y <             0) ship.y += oled.height();
  if (ship.y > oled.height()) ship.y -= oled.height();

  oled.drawLine(
    ship.x, ship.y,
    ship.x + ship.sig * SHIPSIZE * cos(PI * (ship.angle+40)/360.0),
    ship.y + ship.sig * SHIPSIZE * sin(PI * (ship.angle+40)/360.0),
    WHITE
  );
  oled.drawLine(
    ship.x, ship.y,
    ship.x + ship.sig * SHIPSIZE * cos(PI * (ship.angle-40)/360.0),
    ship.y + ship.sig * SHIPSIZE * sin(PI * (ship.angle-40)/360.0),
    WHITE
  );

  oled.drawLine(
    ship.x + ship.sig * (SHIPSIZE-2) * cos(PI * (ship.angle+40)/360.0),
    ship.y + ship.sig * (SHIPSIZE-2) * sin(PI * (ship.angle+40)/360.0),
    ship.x + ship.sig * (SHIPSIZE-2) * cos(PI * (ship.angle-40)/360.0),
    ship.y + ship.sig * (SHIPSIZE-2) * sin(PI * (ship.angle-40)/360.0),
    WHITE
  );
}

void setup() {
  pinMode(BTN_UP,   INPUT_PULLUP);
  pinMode(BTN_DOWN, INPUT_PULLUP);
  pinMode(BTN_RIGHT,INPUT_PULLUP);
  pinMode(BTN_LEFT, INPUT_PULLUP);
  pinMode(BTN_A,    INPUT_PULLUP);
  pinMode(BTN_B,    INPUT_PULLUP);
  pinMode(BTN_SW,   INPUT);
  
  randomSeed(analogRead(RANDIN));

  ship.x=64;
  ship.y=32;
  ship.angle=180;
  ship.sp=0;
  ship.sig=1;

  for (int i=0;i<MAXROCKS;++i) {
    rocks[i].x = random(0, oled.width());
    rocks[i].y = random(0, oled.height());
    rocks[i].angle = 18 * random(0, 20);
    rocks[i].sp = random(1, 4);
  }
  
  oled.begin(SSD1306_SWITCHCAPVCC); // SPI
  // oled.begin(SSD1306_SWITCHCAPVCC, 0x3C); // i2c
  oled.setTextColor(WHITE);
  oled.setTextSize(1);
}

void loop() {
  oled.clearDisplay();

  printShip();
    
  if (digitalRead(BTN_UP) == LOW) {
    ship.sp++;
  }
  if (digitalRead(BTN_DOWN) == LOW) {
    ship.sp--;
  }
  if (digitalRead(BTN_RIGHT) == LOW) {
    ship.angle+=10;
  }
  if (digitalRead(BTN_LEFT) == LOW) {
    ship.angle-=10;
  }
  if (digitalRead(BTN_A) == LOW) {
    buls[buid] = ship;
    buls[buid].sp += 3;
    buls[buid].tick = 1;
    buid++;
    if (buid >= MAXBULLETS) buid=0;
  }
    
  if (ship.angle < 0) {
    ship.angle += 360;
    ship.sig *= -1;
  }
  if (ship.angle >=  360) {
    ship.angle -= 360;
    ship.sig *= -1;
  }
  if (ship.sp < 0)            ship.sp = 0;
  if (ship.sp > SHIPMAXSPEED) ship.sp = SHIPMAXSPEED;
  
  printBullets();
  printRocks();

  oled.setCursor(0,0);
  // score
  oled.display();
  
  delay(100);
}
