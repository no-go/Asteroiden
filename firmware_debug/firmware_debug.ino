#include <SPI.h> //SPI Arduino Library
//#include <Wire.h> //I2C Arduino Library

#include <Adafruit_GFX.h>
#include "Adafruit_SSD1306.h"


#define BTN_UP       A1
#define BTN_DOWN     A0
#define BTN_RIGHT    2
#define BTN_LEFT     3
#define BTN_A        7
#define BTN_B        4
#define BTN_SW       6

// OLED (11 -> MOSI/DIN, 13 ->SCK)
#define PIN_CS     8
#define PIN_DC     9
#define PIN_RESET  10 
Adafruit_SSD1306 oled(PIN_DC, PIN_RESET, PIN_CS);

// HARDWARE I2C: A4 -> SDA, A5 -> SCL
//Adafruit_SSD1306 oled(PIN_RESET);

#define SHIPSIZE      7
#define SHIPMAXSPEED 12
#define BULLETTIME   16

int angle = 0;
int x=64;
int y=32;
int shipSpeed = 0;
short sig = 1; 

struct Bullet {
  int x;
  int y;
  int angle;
  short sig;
  int sp;
  int tick;
};

Bullet bu;

void printBullet(Bullet & bul) {
  if (bul.tick > BULLETTIME) {
    bul.tick = -1;
    oled.drawPixel(bul.x, bul.y, BLACK);
  }
  if (bul.tick >= 0) {
    oled.drawPixel(bul.x, bul.y, WHITE);
    bul.tick++;
    bul.x = bul.x - bul.sig * bul.sp * cos(PI * bul.angle/360.0);
    bul.y = bul.y - bul.sig * bul.sp * sin(PI * bul.angle/360.0);
    
    if (bul.x <             0) bul.x += oled.width();
    if (bul.x >  oled.width()) bul.x -= oled.width();
    if (bul.y <             0) bul.y += oled.height();
    if (bul.y > oled.height()) bul.y -= oled.height();
    
  }
}

void printShip() {
  oled.drawLine(
    x, y,
    x + sig * SHIPSIZE * cos(PI * angle/360.0),
    y + sig * SHIPSIZE * sin(PI * angle/360.0),
    WHITE
  );

  oled.drawLine(
    x, y,
    x + sig * SHIPSIZE * cos(PI * (angle+45)/360.0),
    y + sig * SHIPSIZE * sin(PI * (angle+45)/360.0),
    WHITE
  );
  oled.drawLine(
    x, y,
    x + sig * SHIPSIZE * cos(PI * (angle-45)/360.0),
    y + sig * SHIPSIZE * sin(PI * (angle-45)/360.0),
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
  
  oled.begin(SSD1306_SWITCHCAPVCC); // SPI
  // oled.begin(SSD1306_SWITCHCAPVCC, 0x3C); // i2c
  oled.setTextColor(WHITE);
  oled.setTextSize(1);
}

void loop() {
  x = x - sig * shipSpeed * cos(PI * angle/360.0);
  y = y - sig * shipSpeed * sin(PI * angle/360.0);

  oled.clearDisplay();
  oled.setCursor(0,0);
  
  if (digitalRead(BTN_UP) == LOW) {
    oled.print("Up ");
    shipSpeed++;
  }
  if (digitalRead(BTN_DOWN) == LOW) {
    oled.print("Down ");
    shipSpeed--;
  }
  if (digitalRead(BTN_RIGHT) == LOW) {
    oled.print("right ");
    angle+=10;
  }
  if (digitalRead(BTN_LEFT) == LOW) {
    oled.print("left ");
    angle-=10;
  }
  if (digitalRead(BTN_A) == LOW) {
    oled.print("A ");
    bu.x = x;
    bu.y = y;
    bu.sig = sig;
    bu.angle = angle;
    bu.sp = shipSpeed + 3;
    bu.tick = 0;
  }
  if (digitalRead(BTN_B) == LOW) {
    oled.print("B ");
  }
  
  oled.setCursor(0,30);
  if (digitalRead(BTN_SW) == LOW) {
    oled.print("off");
  } else {
    oled.print("on");
  }
  
  if (angle <     0) {
    angle += 360;
    sig *= -1;
  }
  if (angle >=  360) {
    angle -= 360;
    sig *= -1;
  }
  if (shipSpeed < 0)            shipSpeed = 0;
  if (shipSpeed > SHIPMAXSPEED) shipSpeed = SHIPMAXSPEED;
  
  if (x <             0) x += oled.width();
  if (x >  oled.width()) x -= oled.width();
  if (y <             0) y += oled.height();
  if (y > oled.height()) y -= oled.height();
  
  printShip();
  printBullet(bu);
  
  oled.display();
  
  delay(100);
}
