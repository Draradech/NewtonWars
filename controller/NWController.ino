#include <LiquidCrystal.h>
#include <util/atomic.h>

LiquidCrystal lcd(14, 15, 18, 19, 20, 21);

#define LED_R 3
#define LED_G 5
#define LED_B 6

#define LEFT 8
#define RIGHT 10
#define MODE 9
#define RESET 2
#define FIRE 4
#define ENC_A 7
#define ENC_B 16

int8_t pos = 0;
int8_t enc_a_old = 0;
void evaluateEnc()
{
  uint8_t enc_a = !!(PINE & (1 << PE6));
  uint8_t enc_b = (PINB & (1 << PB2));
  if(!enc_b)
  {
    pos += (enc_a_old - enc_a);
  }
  enc_a_old = enc_a;
}

void setup()
{
  Serial.begin(115200);

  lcd.begin(16, 2);

  attachInterrupt(digitalPinToInterrupt(ENC_A), evaluateEnc, CHANGE);

  pinMode(LED_R, OUTPUT);
  pinMode(LED_G, OUTPUT);
  pinMode(LED_B, OUTPUT);

  pinMode(LEFT, INPUT_PULLUP);
  pinMode(RIGHT, INPUT_PULLUP);
  pinMode(MODE, INPUT_PULLUP);
  pinMode(RESET, INPUT_PULLUP);
  pinMode(FIRE, INPUT_PULLUP);
  pinMode(ENC_B, INPUT_PULLUP);
  pinMode(ENC_A, INPUT_PULLUP);
}


char line[17];
char debug[12];
uint8_t mode = 0;
int32_t res = 100000;
uint8_t dspPos = 9;
int32_t angle = 0;
int32_t speed = 1000000;
uint8_t timerCursor;
uint8_t buttonPin[5] = {LEFT, RIGHT, RESET, MODE, FIRE};
uint8_t buttonOld[11] = {1};
uint8_t button[11];
int32_t energy;
uint8_t red, grn, blu;
char serBuffer[64];
int16_t serIndex;
int16_t colorTimer = 20;
int16_t energyTimer = 1;

void loop()
{
  if(energy > speed)
  {
    analogWrite(LED_R, 255 - red / 8);
    analogWrite(LED_G, 255 - grn / 10);
    analogWrite(LED_B, 255 - blu / 12);
  }
  else
  {
    analogWrite(LED_R, 255);
    analogWrite(LED_G, 255);
    analogWrite(LED_B, 255);
  }

  int16_t c = Serial.read();
  if(c != -1)
  {
    serBuffer[serIndex] = c;
    if(c == 10 || c == 13)
    {
      serBuffer[serIndex] = 0;
      if(serIndex == 4)
      {
        energy = 100000 * atoi(serBuffer);
        energyTimer = 53;
      }
      if(serIndex == 11)
      {
        if(serBuffer[3] == ' ' && serBuffer[7] == ' ')
        {
          int16_t mincol = 255;
          red = atoi(serBuffer);
          grn = atoi(serBuffer + 4);
          blu = atoi(serBuffer + 8);
          mincol = min(mincol, red);
          mincol = min(mincol, grn);
          mincol = min(mincol, blu);
          red -= mincol;
          grn -= mincol;
          blu -= mincol;
          colorTimer = 47;
        }
      }
      serIndex = 0;
    }
    else if(serIndex < 63)
    {
      serIndex++;
    }
  }
 
  if(energyTimer > 0)
  {
    energyTimer--;
  }
  else
  {
    Serial.println("u");
    energyTimer = 53;
  }

  if(colorTimer > 0)
  {
    colorTimer--;
  }
  else
  {
    Serial.println("n Controller");
    Serial.println("d 1");
    Serial.println("g");
    colorTimer = 47;
  }


  int8_t inc;
  ATOMIC_BLOCK(ATOMIC_FORCEON)
  {
    inc = pos;
    pos = 0;
  }

  if(mode == 0)
  {
    angle += inc * res;
    while(angle < 0) angle += 36000000;
    while(angle >= 36000000) angle -= 36000000;
  }
  else
  {
    speed += inc * res;
    if(speed < 0) speed = 0;
    if(speed > 20000000) speed = 20000000;
  }

  for(int i = 0; i < 5; ++i)
  {
    button[buttonPin[i]] = digitalRead(buttonPin[i]);
  }

  if(!button[LEFT] && buttonOld[LEFT])
  {
    if(res < 1000000)
    {
      res *= 10;
      dspPos--;
      if(dspPos == 10) dspPos--;
    }
  }

  if(!button[RIGHT] && buttonOld[RIGHT])
  {
    if(res > 1)
    {
      res /= 10;
      dspPos++;
      if(dspPos == 10) dspPos++;
    }
  }

  if(!button[RESET] && buttonOld[RESET])
  {
    mode = 0;
    res = 100000;
    speed = 1000000;
    angle = 0;
    dspPos = 9;
  }

  if(!button[MODE] && buttonOld[MODE])
  {
    mode = !mode;
  }

  if(inc != 0) timerCursor = 0;
  if(++timerCursor > 127) timerCursor = 0;

  sprintf(line, "Angle: %3ld.%.5ld", angle / 100000, angle % 100000);
  if(mode == 0 && timerCursor > 63) line[dspPos] = ' ';
  lcd.setCursor(0, 0);
  lcd.print(debug[0] ? debug : line);

  sprintf(line, "Speed: %3ld.%.5ld", speed / 100000, speed % 100000);
  if(mode == 1 && timerCursor > 63) line[dspPos] = ' ';
  lcd.setCursor(0, 1);
  lcd.print(line);

  if(!button[FIRE] && buttonOld[FIRE])
  {
    if(energy >= speed) energy -= speed;
    sprintf(line, "v %ld.%.5ld", speed / 100000, speed % 100000);
    Serial.println(line);
    sprintf(line, "%ld.%.5ld", angle / 100000, angle % 100000);
    Serial.println(line);
  }

  for(int i = 0; i < 5; ++i)
  {
    buttonOld[buttonPin[i]] = button[buttonPin[i]];
  }
}
