#include <SoftwareSerial.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

#define NOT_RUN 0
#define MUST_RUN 1

SoftwareSerial g_pms_serial(2, 3);
int i2c_address = 0;
int relay_signal = 4;
int fan_status = NOT_RUN;

void PrintDust(LiquidCrystal_I2C lcd, int pm_1_0, int pm_2_5, int pm_10)
{
  lcd.setCursor(0, 0);
  lcd.print("PM1.0 PM2.5 PM10");
  lcd.setCursor(0, 1);
  lcd.print("                ");
  lcd.setCursor(0, 1);
  lcd.print(pm_1_0);
  lcd.setCursor(6, 1);
  lcd.print(pm_2_5);
  lcd.setCursor(12, 1);
  lcd.print(pm_10);
}

void ControlFan(int pm_1_0, int pm_2_5, int pm_10)
{
  int upper_bound = 20;
  int lower_bound = 10;
  
  if (pm_1_0 > upper_bound || \
      pm_2_5 > upper_bound || \
      pm_10 > upper_bound)
    fan_status = MUST_RUN;
  else if (pm_1_0 < lower_bound && \
           pm_2_5 < lower_bound && \
           pm_10 < lower_bound)
    fan_status = NOT_RUN;
  
  if (fan_status == MUST_RUN)
    digitalWrite(relay_signal, HIGH);
  else
    digitalWrite(relay_signal, LOW);
}

void setup() {
  Serial.begin(9600);
  g_pms_serial.begin(9600);
  Wire.begin();

  for (unsigned char address = 0; address < 128; address++) 
  {
    Wire.beginTransmission(address);
    if (Wire.endTransmission() == 0)
    {
      i2c_address = address;
      break ;
    }
  }

  pinMode(relay_signal, OUTPUT);
}

void loop() {
  unsigned char pms[32];
  int check = 0;
  int pm_1_0, pm_2_5, pm_10;

  if (g_pms_serial.available() < 32)
  {
    delay(500);
    return ;
  }

  g_pms_serial.readBytes(pms, 32);

  for (int j = 0; j < 30 ; j++)
      check += pms[j];
  if ((pms[0] != 0x42 || pms[1] != 0x4d) || \
      (pms[30] != (unsigned char)(check >> 8) || \
      pms[31] != (unsigned char)(check)))
    return ;

  pm_1_0 = (pms[10] << 8) + pms[11];
  pm_2_5 = (pms[12] << 8) + pms[13];
  pm_10 = (pms[14] << 8) + pms[15];

  static LiquidCrystal_I2C lcd(i2c_address, 16, 2);
  static char flag_lcd_init = 0;
  if (flag_lcd_init == 0)
  {
    lcd.init();
    lcd.backlight();
    lcd.clear();
    flag_lcd_init = 1;
  }
  
  PrintDust(lcd, pm_1_0, pm_2_5, pm_10);

  ControlFan(pm_1_0, pm_2_5, pm_10);
}
