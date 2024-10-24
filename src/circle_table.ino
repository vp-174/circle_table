/*
 * Проект стола для фотограмметрии на базе Arduino Nano
 * Автор: Vladislav Panov
 * Версия: 0.45
*/

// - добавил вывод текущего поворота в градусах

#include "IRremote.h" //btn OK 38C7
#include <LiquidCrystal.h>

LiquidCrystal lcd(12, 11, 9, 8, 7, 2);

#define IRpin 10

IRrecv irrecv(IRpin); // Указываем пин, к которому подключен приемник
decode_results results;

volatile bool shouldExit = false;

#define reboot() asm("JMP 0")

// Характеристика двигателя,
// количество шагов на один оборот вала
#define MOTOR_STEPS_REVOLUTION 400

// Передаточный коэффициент шестерёнок
#define GEAR_COEFFICIENT 16.205

// Итоговое количество шагов на один оборот столика
#define STEPS_REVOLUTION GEAR_COEFFICIENT * MOTOR_STEPS_REVOLUTION

// Необходимое количество кадров за один оборот
int SHOTS = 40;

// Количество шагов двигателя между снимками
#define STEPS_ON_SHOT STEPS_REVOLUTION/SHOTS

// Выдержка времени между шагами.
// Чем больше это число, тем медленнее вращается двигатель
int DELAY_TIME = 10;

// Назначим пины
// Шаг двигателя
#define STEP 3
// Направление вращения
#define DIR  4
// Включение двигателя
#define EN   5

// Реле
#define SHOT 6

int blinkL = 0;
bool debug = 1;

int totalSteps = 0; // Общее количество сделанных шагов
float currentAngle = 0.0; // Инициализация переменной currentAngle

float calculateCurrentAngle(int totalSteps) {
  return (float(totalSteps) * 360) / float(STEPS_REVOLUTION);
}

void setup() {
  lcd.begin(16, 2);
  lcd.setCursor(0, 0);
  lcd.print("LOAD >>>");
  lcd.blink();

  digitalWrite(SHOT, HIGH);
  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(DIR, OUTPUT);
  
  Serial.begin(9600);

  irrecv.enableIRIn(); // прием ИК

  // Настроим все необходимые пины на выход
  for (int i = STEP; i <= SHOT; ++i) {
    delay(10);
    pinMode(i, OUTPUT);
  }
}

void loop() {
  // Мигание точкой
  while (blinkL < 3) {
    unsigned long blinkTime = millis();
    digitalWrite(LED_BUILTIN, HIGH);
    while (millis() - blinkTime < 200);
    blinkTime = millis();
    digitalWrite(LED_BUILTIN, LOW);
    while (millis() - blinkTime < 200);
    blinkTime = millis();
    blinkL++;
    lcd.clear();
  }

  lcd.setCursor(0, 0);
  lcd.print("READY >>");
  lcd.noBlink();

// Чтение кнопок
  if (irrecv.decode(&results)) {
    int res = results.value;

      // DEBUG
    if (debug  == 1) Serial.println(res, HEX);

    if (res == 0xFFFFA25D) {
      // Съемка 36 кадров, задержка 10; кнопка [1]
      shootPhotos(36,10);
    }

    if (res == 0x22DD) {
      // Съемка 24 кадров, задержка 12; кнопка [4]
      shootPhotos(24,8);
    }

    if (res == 0xFFFFE01F) {
      // Съемка 12 кадров, задержка 6; кнопка [7]
      shootPhotos(12,6);
    }

    if (res == 0x10EF) {
      // Видео; кнопка [<]
      shootVideo(1,8);
    }

    if (res == 0x6897) {
      // Видео; кнопка [*]
      shootVideo(1,2);
    }

    if (res == 0xFFFFE21D) {
      // Обратная съемка 36 кадров, задержка 10; кнопка [3]
      reverseShootPhotos(36,10);
    }

    if (res == 0xFFFFC23D) {
      // Обратная съемка 24 кадров, задержка 12; кнопка [6]
      reverseShootPhotos(24,8);
    }

    if (res == 0xFFFF906F) {
      // Обратная съемка 12 кадров, задержка 6; кнопка [9]
      reverseShootPhotos(12,6);
    }

    if (res == 0x5AA5) {
      // Видео; кнопка [>]
      reverseShootVideo(1,8);
    }

    if (res == 0xFFFFB04F) {
      // Видео; кнопка [#]
      reverseShootVideo(1,2);
    }
    irrecv.resume(); // Принимаем следующую команду
  }
}

// Прерывание циклов для СТОПА
bool checkForIRSignal() {
  if (irrecv.decode(&results)) {
    int res = results.value;
    irrecv.resume(); // Принимаем следующую команду
    return (res == 0x38C7); // STOP; кнопка [OK]
  }
  return false;
}

// Функция СТОПА
void stopStep(){
    if (checkForIRSignal()) {
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("STOP >>>");
      unsigned long stopTime = millis();
      while (millis() - stopTime < 2500) break;
      reboot();
    }
}

// Фото
void shootPhotos(int SHOTS, int DELAY_TIME) {
  digitalWrite(EN, LOW);
  digitalWrite(DIR, HIGH);
  unsigned long startTime = millis(); // Записываем начальное время

  while (millis() - startTime < 1000);

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("WORK <<<");

  for (int i = 0; i < SHOTS; ++i) {
    unsigned long shotStartTime = millis(); // Записываем начальное время для каждого снимка

    while (millis() - shotStartTime < 1000);
    shotStartTime = millis();
    digitalWrite(SHOT, HIGH);

    while (millis() - shotStartTime < 200);
    shotStartTime = millis();
    digitalWrite(SHOT, LOW);

    lcd.setCursor(0, 1);
    lcd.print("EST:");

    if (SHOTS - i < 10) {
      lcd.setCursor(5, 1);
      lcd.print("  ");
      lcd.setCursor(5, 1);
      lcd.print("0");
      lcd.setCursor(6, 1);
      lcd.print(SHOTS - i);
    } else {
      lcd.setCursor(5, 1);
      lcd.print(SHOTS - i);
    }

    while (millis() - shotStartTime < 3000);
    shotStartTime = millis();
    unsigned long stepStartTime;
    for (int j = 0; j < STEPS_ON_SHOT; ++j) {
      stopStep();

      digitalWrite(STEP, HIGH);
      stepStartTime = millis(); // Записываем время начала шага
      while (millis() - stepStartTime < DELAY_TIME);

      digitalWrite(STEP, LOW);
      stepStartTime = millis(); // Записываем время начала следующего этапа
      while (millis() - stepStartTime < DELAY_TIME);
    }
  }

  lcd.clear();
  digitalWrite(EN, HIGH);
  digitalWrite(SHOT, HIGH);
}

// Фото РЕВЕРС
void reverseShootPhotos(int SHOTS, int DELAY_TIME) {
  digitalWrite(EN, LOW);
  digitalWrite(DIR, LOW);
  unsigned long startTime = millis(); // Записываем начальное время

  while (millis() - startTime < 1000);

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("WORK >>>");

  for (int i = 0; i < SHOTS; ++i) {
    unsigned long shotStartTime = millis(); // Записываем начальное время для каждого снимка

    while (millis() - shotStartTime < 1000);
    shotStartTime = millis();
    digitalWrite(SHOT, HIGH);

    while (millis() - shotStartTime < 200);
    shotStartTime = millis();
    digitalWrite(SHOT, LOW);

    lcd.setCursor(0, 1);
    lcd.print("EST:");

    if (SHOTS - i < 10) {
      lcd.setCursor(5, 1);
      lcd.print("  ");
      lcd.setCursor(5, 1);
      lcd.print("0");
      lcd.setCursor(6, 1);
      lcd.print(SHOTS - i);
    } else {
      lcd.setCursor(5, 1);
      lcd.print(SHOTS - i);
    }

    while (millis() - shotStartTime < 3000);
    shotStartTime = millis();

    unsigned long stepStartTime;
    for (int j = 0; j < STEPS_ON_SHOT; ++j) {
      stopStep();

      digitalWrite(STEP, HIGH);
      stepStartTime = millis(); // Записываем время начала шага
      while (millis() - stepStartTime < DELAY_TIME);

      digitalWrite(STEP, LOW);
      stepStartTime = millis(); // Записываем время начала следующего этапа
      while (millis() - stepStartTime < DELAY_TIME);
    }
  }

  lcd.clear();
  digitalWrite(EN, HIGH);
  digitalWrite(SHOT, HIGH);
}

// Видео
void shootVideo(int SHOTS, int DELAY_TIME) {
  digitalWrite(EN, LOW);
  digitalWrite(DIR, HIGH);

  unsigned long startTime = millis(); // Записываем начальное время

  while (millis() - startTime < 1000);

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("WORK <<<");
  for (int i = 0; i < SHOTS; ++i) {
    stopStep();
    lcd.setCursor(0, 1);
    lcd.print("DEG:");
  
    unsigned long stepStartTime;
    for (int j = 0; j < STEPS_ON_SHOT; ++j) {
      stopStep();

      totalSteps++;
      lcd.setCursor(5, 1);
      currentAngle = calculateCurrentAngle(totalSteps); // Обновляем текущий угол
      lcd.print(int(currentAngle)); // Выводим текущий угол

      digitalWrite(STEP, HIGH);
      stepStartTime = millis(); // Записываем время начала шага
      while (millis() - stepStartTime < DELAY_TIME);

      digitalWrite(STEP, LOW);
      stepStartTime = millis(); // Записываем время начала следующего этапа
      while (millis() - stepStartTime < DELAY_TIME);
    }
  }
  
  lcd.clear();
  digitalWrite(EN, HIGH);
  digitalWrite(SHOT, HIGH);
  reboot();
}

// Видео РЕВЕРС
void reverseShootVideo(int SHOTS, int DELAY_TIME) {
  digitalWrite(EN, LOW);
  digitalWrite(DIR, LOW);
  unsigned long startTime = millis(); // Записываем начальное время

  while (millis() - startTime < 1000);

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("WORK >>>");
  for (int i = 0; i < SHOTS; ++i) {
    stopStep();
    lcd.setCursor(0, 1);
    lcd.print("EST:");

    unsigned long stepStartTime;
    for (int j = 0; j < STEPS_ON_SHOT; ++j) {
      stopStep();
      
      totalSteps++;
      lcd.setCursor(5, 1);
      currentAngle = calculateCurrentAngle(totalSteps); // Обновляем текущий угол
      lcd.print(int(currentAngle)); // Выводим текущий угол

      digitalWrite(STEP, HIGH);
      stepStartTime = millis(); // Записываем время начала шага
      while (millis() - stepStartTime < DELAY_TIME);

      digitalWrite(STEP, LOW);
      stepStartTime = millis(); // Записываем время начала следующего этапа
      while (millis() - stepStartTime < DELAY_TIME);
    }
  }

  lcd.clear();
  digitalWrite(EN, HIGH);
  digitalWrite(SHOT, HIGH);
  reboot();
}