// Работа с дисплеем, часами и матрицей.
#include <QuadDisplay2.h>
#include <iarduino_RTC.h>
#include <Adafruit_NeoPixel.h>
// Для работы шины I2C - для подключения часов.
#include <Wire.h>

// Объявление дисплея, часов и матрицы.
QuadDisplay display(10);
iarduino_RTC watch(RTC_DS1307);
Adafruit_NeoPixel matrix(16, 8, NEO_GRB + NEO_KHZ800);

// Пины кнопок и пьезопищалки.
byte button1(0), button2(1), button3(4), button4(5);
byte squeaker(7);

// Выводимые и сохраняемые показатели времени.
byte hours, minutes, seconds, weekday;
// Текущий будильник и его время пробуждения.
byte hourUp(100), minuteUp(100), alarm(11);
// Счётчик для блокировки нажатия кнопок.
unsigned long clicked(0);

// Запись данных будильника в структуру.
struct Alarm
{ 
  // Режим: день недели (1-6,0)/"однократно"/"ежедневно".
  byte mode    = 100;
  byte hour    = 100;
  byte minute  = 100; 
  bool rang    = false;
};
// Хранение десяти будильников.
Alarm alarms[10];

void setup()
{     
  //Serial.begin(9600); /*раскомм для работы Serial*/
  
  // Инициализируются дисплей, часы и матрица.   
  display.begin();
  watch.begin();
  matrix.begin();
  // Матрица очищается.
  clearMatrix();

  // Объявляются кнопки.
  pinMode(button1, INPUT);
  pinMode(button2, INPUT);
  pinMode(button3, INPUT);
  pinMode(button4, INPUT);
  
  //            !!! ручная установка времени !!!
  //  (секунда,минута,час, день,месяц,год, день недели 1-6, 0)
  //          watch.settime(45,56,22, 17,1,21, 0);
  //            раскомм, прошить, закомм, прошить 

  // Вывод "HI".
  display.displayDigits(QD_H, QD_I, QD_NONE, QD_NONE);
  delay(2000);
}

void loop()
{
  // Отображается время.
  showTime();

  //**********************************************//
 /* Serial.println("ALARMS");
  for (byte al = 0; al < 10; ++al)
  {
    Serial.print(alarms[al].mode);
  Serial.print("\t");
  Serial.print(alarms[al].hour);
  Serial.print("\t");
  Serial.print(alarms[al].minute);
  Serial.print("\t");
  Serial.println(alarms[al].rang);
  }
  Serial.print("\t");
  Serial.print(hourUp);
  Serial.print("\t");
  Serial.print(minuteUp);
  Serial.println("\t");*/
  //*********раскомм для проверки данных**********// 
  // НЕ ТРОГАТЬ, если будильник работает исправно!
  //  в противном случае закомм блок с кнопкой 2 !

  // Кнопка 1 - изменение показателя часа.
  if (millis()-clicked > 500 && !digitalRead(button1))
  { 
    changeHour(); 
    // Определяется ближайшее время пробуждения.
    getWakeUpTime();
    clicked = millis(); 
  }

  // Кнопка 4 - изменение показателя минуты.
  if (millis()-clicked > 500 && !digitalRead(button4))
  { 
    changeMinute(); 
    // Определяется ближайшее время пробуждения.
    getWakeUpTime();
    clicked = millis(); 
  }

  // Кнопка 3 - добавление/удаление будильников.
  if (millis()-clicked > 500 && !digitalRead(button3))
  {
    delay(500);
    // При длительном нажатии будильник удаляется.
    if (millis()-clicked > 500 && !digitalRead(button3))
    { 
      matrixDeleteAlarm();  
      deleteAlarm(); 
      clearMatrix();      
    }
    // Будильник добавляется.
    else
    { 
      matrixSetAlarm();     
      addAlarm(); 
      clearMatrix();
    }

    showTime();
    // Определяется ближайшее время пробуждения.
    getWakeUpTime();
    clicked = millis();
  }

  if (hours == 0 && minutes == 0)
    getWakeUpTime();

  // Включение будильника за 10 минут до пробуждения.
  if (hours*60 + minutes == hourUp*60 + minuteUp - 10)
  {
    // Запускается бионический будильник.
    matrixAlarm();
    clearMatrix();
    alarms[alarm].rang = true;
    // Однократный будильник удаляется после пробуждения.
    if (alarms[alarm].mode == 7)
      deleteAlarm(alarm);
      
    // Определяется новое время пробуждения.
    getWakeUpTime();
  }
  
  // Кнопка 2 - вывод установленных будильников.
  if (millis()-clicked > 500 && !digitalRead(button2))
  {
    // Включается режим отображения.
    matrixShowAlarms();
    
    delay(500);
    // При длительном нажатии показываются все будильники.
    if (millis()-clicked > 500 && !digitalRead(button2))
          { showAlarms(); }
    // Показываются будильники на "сегодня".
    else  { showTodayAlarms(); }

    clearMatrix();
    clicked = millis();
  }
}

//--------------------ФУНКЦИИ РАБОТЫ С МАТРИЦЕЙ--------------------//

// Очистка светодиодной матрицы.
void clearMatrix()
{
  for (byte pix = 0; pix < matrix.numPixels(); pix++) 
  {
    matrix.setPixelColor(pix, 0);
    matrix.show();
  }
}

// Отображение режима настройки будильника.
void matrixSetAlarm()
{
  // 4 жёлтых светодиода в середине.
  matrix.setBrightness(8); 
  matrix.show();
  matrix.setPixelColor(5,   matrix.Color(225, 225, 0)); 
  matrix.show();
  matrix.setPixelColor(6,   matrix.Color(225, 225, 0)); 
  matrix.show();
  matrix.setPixelColor(9,   matrix.Color(225, 225, 0)); 
  matrix.show();
  matrix.setPixelColor(10,  matrix.Color(225, 225, 0)); 
  matrix.show();
}

// Отображение режима удаления будильника.
void matrixDeleteAlarm()
{
  // 4 красных светодиода в середине.
  matrix.setBrightness(8); 
  matrix.show();
  matrix.setPixelColor(5,   matrix.Color(225, 0, 0)); 
  matrix.show();
  matrix.setPixelColor(6,   matrix.Color(225, 0, 0)); 
  matrix.show();
  matrix.setPixelColor(9,   matrix.Color(225, 0, 0)); 
  matrix.show();
  matrix.setPixelColor(10,  matrix.Color(225, 0, 0)); 
  matrix.show();
}

// Отображение режима просмотра будильников.
void matrixShowAlarms()
{
  // 4 голубых светодиода в середине.
  matrix.setBrightness(8); 
  matrix.show();
  matrix.setPixelColor(5,   matrix.Color(0, 255, 255)); 
  matrix.show();
  matrix.setPixelColor(6,   matrix.Color(0, 255, 255)); 
  matrix.show();
  matrix.setPixelColor(9,   matrix.Color(0, 255, 255)); 
  matrix.show();
  matrix.setPixelColor(10,  matrix.Color(0, 255, 255)); 
  matrix.show();
}

// Отображение галочки зелёного цвета.
void matrixDone()
{
  clearMatrix();
  matrix.setBrightness(16); 
  matrix.show();
  matrix.setPixelColor(3, matrix.Color(0, 64, 0, 64)); 
  matrix.show();
  matrix.setPixelColor(4, matrix.Color(0, 64, 0, 64)); 
  matrix.show();
  matrix.setPixelColor(6, matrix.Color(0, 64, 0, 64)); 
  matrix.show();
  matrix.setPixelColor(9, matrix.Color(0, 64, 0, 64)); 
  matrix.show();  

  // Матрица очищается.
  delay(1000); clearMatrix();
}

// Работа будильника.
void matrixAlarm()
{
  // Бионический будильник - постепенный "рассвет".
  for (byte bright = 5; bright < 255; ++bright)
  {
    showTime();
    matrix.setBrightness(bright); 
    matrix.show();

    // Установка каждого пикселя белым.
    for (byte pix = 0; pix < 16; pix++) 
    {
      matrix.setPixelColor(pix, matrix.Color(255, 255, 255));
      matrix.show();
    }

    long now = millis();
    // Ожидание 2400мс до следующей итерации.
    while (millis()-now <= 2400)
    {
      if (millis()-clicked > 500 && !digitalRead(button1))
      { clicked = millis(); return; }
    }
  } 

  int notes[10] = { 261, 277, 294, 311, 330, 349, 370, 392, 415, 440 };

  // Звуковой будильник.
  while (true)
  {
    // Примитивная мелодия.
    for (byte note = 0; note < 10; ++note)
    {  
      tone(squeaker, notes[note], 250); 
      delay(250); 
      
      if (millis()-clicked > 500 && !digitalRead(button1))
      { clicked = millis(); return; }
    }
  }
}

//--------------------ФУНКЦИИ РАБОТЫ С ЧАСАМИ--------------------//

// Отображение реального времени.
void showTime()
{
  // Доступ к времени с часов реального времени.
  watch.gettime();
  hours   = watch.Hours;
  minutes = watch.minutes;
  seconds = watch.seconds;
  weekday = watch.weekday;

  // Вывод на дисплей.
  display.displayScore(hours,minutes, true);
}

// Настройка текущего часа.
void changeHour()
{
  delay(500);
  // При длительном нажатии - час-1.
  if (millis()-clicked > 500 && !digitalRead(button1))
  {
    if (hours > 0) 
    watch.settime(0,-1,hours-1);
    // До 0 - 23.
    else watch.settime(0,-1,23);

    clicked = millis();
  }
  // Час+1.
  else
  {
    if (hours < 23) 
    watch.settime(0,-1,hours+1);
    // После 23 - 0.
    else watch.settime(0,-1,0);
  }
}

// Настройка текущей минуты.
void changeMinute()
{
  delay(500);
  // При длительном нажатии - минута-1.
  if (millis()-clicked > 500 && !digitalRead(button4))
  {
    if (minutes > 0) 
    watch.settime(0,minutes-1);
    // До 0 - 59.
    else watch.settime(0,59);
  }
  // Минута+1.
  else
  {
    if (minutes < 59) 
    watch.settime(0,minutes+1);
    // После 59 - 0.
    else watch.settime(0,0);
  }    
}

//--------------------ФУНКЦИИ РАБОТЫ С БУДИЛЬНИКАМИ--------------------//

// Добавление будильника.
void addAlarm()
{
  display.displayDigits(QD_A, QD_d, QD_d, QD_NONE);
  delay(2000);

  bool avail = false;
  // Поиск свободного будильника из десяти.
  for (byte al = 0; al < 10; ++al)
  {
    if (alarms[al].mode == 100)
    { 
      // Настраиваются параметры найденного будильника.
      setAlarm(al); 
      avail = true; break;
    }
  }
  // При отстуствии свободных перенастраивается первый.
  if (!avail) setAlarm(0);
}

// Настройка добавляемого будильника.
void setAlarm(byte al)
{
  // Устанавливается режим будильника.
  byte aM = setAlarmMode(0);
  if      (aM == 0) alarms[al].mode = 7;
  else if (aM == 7) alarms[al].mode = 0;
  else              alarms[al].mode = aM;

  hours   = 0; minutes = 0;
  // Настраивается время добавляемого будильника.
  while (true)
  {
    // Вывод времени будильника на дисплей.
    display.displayScore(hours,minutes, true);
    
    // Кнопка 1 - изменение показателя часа.
    if (millis()-clicked > 500 && !digitalRead(button1))
    { chooseHour();   clicked = millis(); }

    // Кнопка 4 - изменение показателя минуты.
    if (millis()-clicked > 500 && !digitalRead(button4))
    { chooseMinute(); clicked = millis(); }

    // Кнопка 2 - утверждение будильника.
    if (millis()-clicked > 500 && !digitalRead(button2))
    {
      alarms[al].hour   = hours;
      alarms[al].minute = minutes;

      matrixDone();      
      clicked = millis(); break;
    }
  }
}

// Выбор режима будильника.
byte setAlarmMode(byte m)
{
  // На дисплей выводится предлагаемый режим.
  showMode(m); if (m > 8) return 0;

  // Переключение на следующий режим, либо утверждение текущего.
  while (true)
  {
    if (millis()-clicked > 500 && !digitalRead(button2)) 
    { clicked = millis(); return m; }
    
    if (millis()-clicked > 500 && !digitalRead(button4))
    {
      clicked = millis();
      return setAlarmMode(++m);
    }
  }
}

// Установка часа будильника.
void chooseHour()
{
  delay(500);
  // При длительном нажатии - час-10/час+14.
  if (millis()-clicked > 500 && !digitalRead(button1))
  {
    if (hours >= 10) hours -= 10;
    else hours += 14;
  }
  // Час+1.
  else
  {
    if (hours < 23) ++hours;
    else hours = 0;
  }
}

// Установка минуты будильника.
void chooseMinute()
{
  delay(500);
  // При длительном нажатии - минута-10.
  if (millis()-clicked > 500 && !digitalRead(button4))
  {
    if (minutes >= 10) minutes -= 10;
    else minutes += 50;
  }
  // Минута+1.
  else
  {
    if (minutes < 59) ++minutes;
    else minutes = 0;
  }
}

// Поиск ближайшего подходящего будильника.
void getWakeUpTime()
{
  alarm = 11; hourUp = 100; minuteUp = 100;
  
  for (byte al = 0; al < 10; ++al)
  {
    // Режимы "однократно"/"ежедневно"/"текущий" день недели.
    if (!alarms[al].rang && (alarms[al].mode == weekday || 
      alarms[al].mode == 7 || alarms[al].mode == 8))
    {
      // Устанавливается время ближайшего будильника.
      if (alarms[al].hour*60 + alarms[al].minute < hourUp*60 + minuteUp
       && alarms[al].hour*60 + alarms[al].minute >= hours*60 + minutes + 10)
      { setWakeUpTime(al); alarm = al; }
    }
  }
}

// Установка времени пробуждения будильника.
void setWakeUpTime(byte al)
{
  hourUp   = alarms[al].hour;
  minuteUp = alarms[al].minute;
}

// Запрос об удалении группы будильников.
void deleteAlarm()
{
  // Выбирается режим будильников.
  byte aM = setAlarmMode(0);
  if      (aM == 0) aM = 7;
  else if (aM == 7) aM = 0;
  // Эти будильники удаляются.
  for (byte al = 0; al < 10; ++al)
    if (alarms[al].mode == aM)
      deleteAlarm(al);

  matrixDone();
}

// Удаление указанного будильника.
void deleteAlarm(byte al)
{
  // Возврат к "нулевым показателям".
  alarms[al].mode   = 100;
  alarms[al].hour   = 100;
  alarms[al].minute = 100;
  alarms[al].rang   = false;
}

//--------------------РЕЖИМЫ БУДИЛЬНИКА----------------------//

// Вывод на дисплей.
void showMode(byte m)
{
  // 1 t    - "однократно".
  // nnon   - "Понедельник".
  // tuES   - "Вторник".
  // uuEd   - "Среда".
  // tHur   - "Четверг".
  // FrId   - "Пятница".
  // Sat    - "Суббота".
  // Sun    - "Воскресенье".
  // Eu d   - "ежедневно".
  if (m == 0) display.displayDigits(QD_1,   QD_NONE,  QD_t,     QD_NONE);
  if (m == 1) display.displayDigits(QD_n,   QD_n,     QD_o,     QD_n);
  if (m == 2) display.displayDigits(QD_t,   QD_u,     QD_E,     QD_S);
  if (m == 3) display.displayDigits(QD_u,   QD_u,     QD_E,     QD_d);
  if (m == 4) display.displayDigits(QD_t,   QD_h ,     QD_u,     QD_r);
  if (m == 5) display.displayDigits(QD_F,   QD_r,     QD_I,     QD_d);
  if (m == 6) display.displayDigits(QD_S,   QD_a,     QD_t,     QD_NONE);
  if (m == 7) display.displayDigits(QD_S,   QD_u,     QD_n,     QD_NONE);
  if (m == 8) display.displayDigits(QD_E,   QD_u,     QD_NONE,  QD_d);
}

//--------------------ФУНКЦИИ РАБОТЫ С ДИСПЛЕЕМ--------------------//

// Вывод информации об установленных будильниках.
void showAlarms()
{
  bool empty = true;
  for (byte al = 0; al < 10; ++al)
  {
    // Проверка будильника на "присутствие".
    if (alarms[al].mode != 100)
    {
      empty = false;

      // Показываются режимы установленных будильников.
      if      (alarms[al].mode == 7)  showMode(0);
      else if (alarms[al].mode == 0)  showMode(7);
      else                            showMode(alarms[al].mode);
      delay(2000);

      // Показывается их время.
      display.displayScore(alarms[al].hour, alarms[al].minute, true);
      delay(2000);
    }
  }

  // Выводится "no a" - будильников нет.
  if (empty) noAlarms();
}

// Вывод информации о будильниках на сегодня.
void showTodayAlarms()
{
  bool empty = true;
  for (byte al = 0; al < 10; ++al)
  {
    // Проверка будильника на "сегодняшний" день.
    if (alarms[al].mode == weekday || alarms[al].mode == 8 ||
      (alarms[al].mode == 7 && (alarms[al].hour > hours || 
      (alarms[al].hour == hours && alarms[al].minute > minutes))))
    {
      empty = false;

      // Показываются режимы подходящих будильников.
      if      (alarms[al].mode == 7)  showMode(0);
      else if (alarms[al].mode == 0)  showMode(7);
      else                            showMode(alarms[al].mode);
      delay(2000);

      // Показывается их время.
      display.displayScore(alarms[al].hour, alarms[al].minute, true);
      delay(2000);
    }
  }

  // Выводится "no a" - будильников нет.
  if (empty) noAlarms();
}

// Вывод "no a".
void noAlarms()
{
  display.displayDigits(QD_n, QD_o, QD_NONE, QD_a);
  delay(2000);
}
