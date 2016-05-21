#include <EEPROM.h>
#include "EEPROMAnything.h"
#include <ClickEncoder.h>
#include <TimerOne.h>
#include <LiquidCrystal.h>

//Controle du moteur
#define moteurPin 3

// LCD definition
#define LCD_light 9
#define LCD_RS      12
#define LCD_RW      11
#define LCD_EN      10
#define LCD_D4       4
#define LCD_D5       5
#define LCD_D6       6
#define LCD_D7       7
#define LCD_CHARS   16
#define LCD_LINES    2
LiquidCrystal lcd(LCD_RS, LCD_RW, LCD_EN, LCD_D4, LCD_D5, LCD_D6, LCD_D7);

/* Useful Constants */
#define SECS_PER_MIN  (60UL)
#define SECS_PER_HOUR (3600UL)
#define SECS_PER_DAY  (SECS_PER_HOUR * 24L)
/* Useful Macros for getting elapsed time */
#define numberOfSeconds(_time_) (_time_ % SECS_PER_MIN)
#define numberOfMinutes(_time_) ((_time_ / SECS_PER_MIN) % SECS_PER_MIN)
#define numberOfHours(_time_) (( _time_% SECS_PER_DAY) / SECS_PER_HOUR)
#define elapsedDays(_time_) ( _time_ / SECS_PER_DAY)

// Configuration sauvegardee en EEPROM
struct config_t
{
  long waitingTime;
  int quantite;
} configuration;

ClickEncoder *encoder;
int16_t value = 0, last = 0;

char ligne1[16] = "";
char ligne2[16] = "";

int menu = 0;

void timerIsr() {
  encoder->service();
}

long correction, remainingTime, lastDistributionTime , elapsedLightTime, lastActionTime = 0;
long lightDuration = 10;

void printRemainingTime(long val) {
  int days = elapsedDays(val);
  int hours = numberOfHours(val);
  int minutes = numberOfMinutes(val);
  int seconds = numberOfSeconds(val);
  sprintf(ligne1, "Prochaine dans:");
  sprintf(ligne2, "%dJ et %.2d:%.2d:%.2d", days, hours, minutes, seconds);
  lcd.setCursor(0, 0);
  lcd.print(ligne1);
  lcd.setCursor(0, 1);
  lcd.print(ligne2);
}

void printDistribution() {
  lcd.setCursor(0, 0);
  lcd.print("  Distribution  ");
  lcd.setCursor(0, 1);
  lcd.print("    en cours    ");
}

void printWaitingTimeConfiguration() {
  long val = configuration.waitingTime;
  int days = elapsedDays(val);
  int hours = numberOfHours(val);
  int minutes = numberOfMinutes(val);
  int seconds = numberOfSeconds(val);
  sprintf(ligne1, "Attente repas: ");
  sprintf(ligne2, "%dJ et %.2d:%.2d:%.2d", days, hours, minutes, seconds);
  lcd.setCursor(0, 0);
  lcd.print(ligne1);
  lcd.setCursor(0, 1);
  lcd.print(ligne2);
}

void printQuantityConfiguration() {
  sprintf(ligne1, "Quantite croket ");
  sprintf(ligne2, "      %3ds    ", configuration.quantite);
  lcd.setCursor(0, 0);
  lcd.print(ligne1);
  lcd.setCursor(0, 1);
  lcd.print(ligne2);
}

void switchMenu() {
  Serial.println("Switch");
  menu += 1;
  menu = menu % 3;
  Serial.println(menu);
  lastActionTime = millis() / 1000;
}

void setup() {
  Serial.begin(9600);
  Serial.println("Starting");
  EEPROM_readAnything(0, configuration);
  if (configuration.waitingTime < 60){
    configuration.waitingTime = 60;
  }
  pinMode(moteurPin, OUTPUT);
  pinMode(LCD_light, OUTPUT); 
  analogWrite(moteurPin, 0);
  encoder = new ClickEncoder(A1, A0, A2, 4);
  encoder->setAccelerationEnabled(true);

  lcd.begin(LCD_CHARS, LCD_LINES);
  lcd.clear();

  Timer1.initialize(1000);
  Timer1.attachInterrupt(timerIsr);

}

void loop() {
  value = encoder->getValue();
  if (value != 0){
    lastActionTime = millis() / 1000;
  }

  // Gestion du menu
  switch (menu) {
    case 0:
      printRemainingTime(remainingTime);
      correction += value*60;
      break;
    case 1:
      if (value != 0) {
        configuration.waitingTime += value*60*5;
        if (configuration.waitingTime<300){
          configuration.waitingTime = 300;
        }
        EEPROM_writeAnything(0, configuration);
      }
      printWaitingTimeConfiguration();
      break;
    case 2:
      if (value != 0) {
        configuration.quantite += value;
        EEPROM_writeAnything(0, configuration);
      }
      printQuantityConfiguration();
      break;
  }

  // Gestion du decompte de temps et de la distribution de croquette
  remainingTime = configuration.waitingTime - ((millis() / 1000) - lastDistributionTime) + correction;
  if (remainingTime <= 0) {
    printDistribution();
    analogWrite(moteurPin, 255); // Activation de la vis sans fin
    delay(configuration.quantite * 1000);
    lastDistributionTime = millis() / 1000;
    correction = 0;
    analogWrite(moteurPin, 0); // Arret de la vis
  }

    // Gestion de l'allumage de l'ecran
    elapsedLightTime = (millis() / 1000) - lastActionTime - lightDuration;
  if (elapsedLightTime > 0) {
    Serial.print("Light Off ");
    Serial.println(elapsedLightTime);
    digitalWrite(LCD_light, LOW); // Stop de la lumiere 
  } else {
    Serial.print("Light On ");
    Serial.println(elapsedLightTime);
    digitalWrite(LCD_light, HIGH); // Allume de la lumiere
  }

  // Gestion du push
  ClickEncoder::Button b = encoder->getButton();
  if (b != ClickEncoder::Open) {
    switch (b) {
      case ClickEncoder::Clicked: switchMenu(); break;

    }
  }
}

