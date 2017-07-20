#include <SPI.h> //SPI Arduino Library
//#include <Wire.h> //I2C Arduino Library

#include <EEPROM.h>
int eeAddress = 1;

#include <Adafruit_GFX.h>
#include "Adafruit_SSD1306.h"


#define BTN_UP       A1
#define BTN_DOWN     A0
#define BTN_RIGHT    2
#define BTN_LEFT     3
#define BTN_A        7 // Fire
#define BTN_B        4 // unused
#define BTN_SW       6 // unused

#define BEEPER       5

#define RANDIN       A7

// OLED (11 -> MOSI/DIN, 13 ->SCK)
#define PIN_CS     8
#define PIN_DC     9
#define PIN_RESET  10 
Adafruit_SSD1306 oled(PIN_DC, PIN_RESET, PIN_CS);

// HARDWARE I2C: A4 -> SDA, A5 -> SCL
//Adafruit_SSD1306 oled(PIN_RESET);

#define SHIPSIZE     10
#define SHIPMAXSPEED 12
#define BULLETTIME   20
#define MAXBULLETS    7
#define MAXROCKS     27 // 3*3*3
#define MAXLIVES     3

struct Propa {
  short x;
  short y;
  short angle;
  short   sig;
  short    sp;
  short  tick; // timer vor bullets & maybe identify a smaller rock (?)
};

byte    lives = MAXLIVES;
int     score = 0;
int highscore = 0;
bool died = false;

Propa ship;

Propa buls[MAXBULLETS];
byte buid = 0;

short freq;

Propa rocks[MAXROCKS];

static const uint8_t names[] = {'c', 'd', 'e', 'f', 'g', 'a', 'b', 'C'};  
static const short   tones[] = {1915, 1700, 1519, 1432, 1275, 1136, 1014, 956};
static const uint8_t melody[] = "2d2a1f2c2d2a2d2c2f2d2a2c";

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

inline void shipInit() {
  ship.x=oled.width()/2;
  ship.y=oled.height()/2;
  ship.angle=180;
  ship.sp=0;
  ship.sig=1;  
}

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

bool printRocks() {
  for (int i=0; i < MAXROCKS; ++i) {
    if (rocks[i].tick <= 0) continue; // hide

    // ------------------ ship collision?
      if (
        rocks[i].x >= (ship.x-3) && rocks[i].x <= (ship.x+3) &&
        rocks[i].y >= (ship.y-3) && rocks[i].y <= (ship.y+3)
      ) {
        shipInit();
        lives--;
        if (lives == 0) return true;
      }
    // ------------------ ship collision? END
    
    if (rocks[i].tick == 1) {
      oled.drawBitmap(rocks[i].x -7, rocks[i].y -7, rock2, 16,16, WHITE);          
    } else if (rocks[i].tick == 2) {
      oled.drawBitmap(rocks[i].x -4, rocks[i].y -4, rock1, 8,8, WHITE);
    } else {
      oled.drawBitmap(rocks[i].x -4, rocks[i].y -2, rock0, 8,4, WHITE);
    }

    rocks[i].x = rocks[i].x + rocks[i].sp * cos(PI * rocks[i].angle/360.0);
    rocks[i].y = rocks[i].y + rocks[i].sp * sin(PI * rocks[i].angle/360.0);
    
    if (rocks[i].x <             0) rocks[i].x += oled.width();
    if (rocks[i].x >  oled.width()) rocks[i].x -= oled.width();
    if (rocks[i].y <             0) rocks[i].y += oled.height();
    if (rocks[i].y > oled.height()) rocks[i].y -= oled.height();
  }
  return false;
}

inline void printLives() {
  for (int i=1; i<=MAXLIVES; ++i) {
    if (i <= lives) {
      oled.setCursor(oled.width()-(i*8), 0);
      oled.print('A');
    }
  }
}

void startGame() {
  score = 0;
  lives = MAXLIVES;
  buid = 0;
  EEPROM.get(eeAddress, highscore);
  
  shipInit();
  
  oled.clearDisplay();
  oled.setCursor(0, 0);
  oled.print("Highscore: ");
  oled.print(highscore);
  
  oled.setCursor(oled.width()/2,oled.height()/2);
  printShip();
  oled.print(" STEROIDEN");
  oled.display();
  
  for (int i=0; i<MAXROCKS; ++i) { // 0,9,18
    rocks[i].x = random(7, oled.width());
    rocks[i].y = random(7, oled.height()-7);
    rocks[i].angle = 18 * random(0, 20);
    rocks[i].sp = random(1, 4);
    rocks[i].tick = -1; // hidden
    if (i%9 == 0) rocks[i].tick = 1; // 0,9,18 not hidden
  }
  
  byte count  = 0;
  byte count2 = 0;
  byte count3 = 0;
  for (count = 0; count < 12; count++) {
    for (count3 = 0; count3 <= (melody[count*2] - 48) * 30; count3++) {
      for (count2=0;count2<8;count2++) {
        if (names[count2] == melody[count*2 + 1]) {      
          analogWrite(BEEPER, 500);
          delayMicroseconds(tones[count2]);
          analogWrite(BEEPER, 0);
          delayMicroseconds(tones[count2]);
        }
      }
    }
  }
  delay(1000);
}

inline void gameOver() {
  for( int i=0;i<500;++i) {
    analogWrite(BEEPER, 500);
    delayMicroseconds(600+i);
    analogWrite(BEEPER, 0);
    delayMicroseconds(600+i);
  }
  oled.clearDisplay();
  oled.setCursor(oled.width()/2 -26,oled.height()/2 - 6);
  oled.print("Game Over");
  oled.display();
  delay(1000);
  oled.setCursor(oled.width()/2 -29,oled.height()/2 + 6);
  oled.print("Score: ");
  oled.print(score);
  oled.display();
  delay(6000);
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

void collisions() {
  for (int i=0; i < MAXBULLETS; ++i) {  
    if (buls[i].tick < 0) continue;
    
    // bullet is active
    for (int j=0; j < MAXROCKS; ++j) {
      if (rocks[j].tick <= 0) continue;
      // is visible
      if (
        //(rocks[j].x == buls[i].x && rocks[j].y == buls[i].y)
        rocks[j].x >= (buls[i].x-3) && rocks[j].x <= (buls[i].x+3) &&
        rocks[j].y >= (buls[i].y-3) && rocks[j].y <= (buls[i].y+3)
      ) {
        score += 3 * ship.sp + 3 * rocks[j].tick; // if ship is fast and small -> more score!
        rocks[j].tick++;
        rocks[j].angle += 15;
        if (rocks[j].tick == 2) {
          rocks[j+3] = rocks[j];
          rocks[j+3].angle += 100;
          rocks[j+3].x += 5;
          rocks[j+6] = rocks[j];
          rocks[j+6].angle -= 100;
          rocks[j+6].y += 5;
        }
        if (rocks[j].tick == 3) {
          rocks[j+1] = rocks[j];
          rocks[j+1].angle += 120;
          rocks[j+1].x += 5;
          rocks[j+2] = rocks[j];
          rocks[j+2].angle -= 120;
          rocks[j+2].y += 5;
        }
        if (rocks[j].tick == 4) rocks[j].tick = -1; // 3 hits? -> make it disapear
      }          
    }
    
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

void setup() {
  pinMode(BTN_UP,   INPUT_PULLUP);
  pinMode(BTN_DOWN, INPUT_PULLUP);
  pinMode(BTN_RIGHT,INPUT_PULLUP);
  pinMode(BTN_LEFT, INPUT_PULLUP);
  pinMode(BTN_A,    INPUT_PULLUP);
  pinMode(BTN_B,    INPUT_PULLUP);
  pinMode(BTN_SW,   INPUT);
  
  randomSeed(analogRead(RANDIN));
  
  oled.begin(SSD1306_SWITCHCAPVCC); // SPI
  // oled.begin(SSD1306_SWITCHCAPVCC, 0x3C); // i2c
  oled.setTextColor(WHITE);
  oled.setTextSize(1);

  startGame();
}

void loop() {
  analogWrite(BEEPER, 0);
  
  oled.clearDisplay();
    
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
    analogWrite(BEEPER, 500);
    delayMicroseconds(1915);
    
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
  
  died = printRocks();
  printBullets();
  printShip();
  printLives();

  collisions();

  oled.setCursor(0,0);
  oled.print(score);
  oled.display();
  
  delay(60);
  if (died) {
    gameOver();
    if (score > highscore) EEPROM.put(eeAddress, score);
    startGame();
  }
}
