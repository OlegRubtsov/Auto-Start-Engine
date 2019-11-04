  // https://github.com/OlegRubtsov/Auto-Start-Engine

// Автозапуск авто на arduino и модеме m590e
// запуск возможен двойным звонком на номер сим карты, оба звонка должны быть сделаны на 30 секунд
// если двигатель запущен, то одиночный звонок продлит время работы
// All commands:  - перечень СМС
// Start          - запуск
// Stop           - остановка
// Balance        - баланс встроенной сим карты
// Info           - краткая информация о системе (статус, напряжение, температура...)
// List           - перечень прописанных телефонов
// P0 79001122331 - установка прописанных номеров, максимум 3 штуки, формат СМС "P0 7900xxxxxxx" или "P1 7900xxxxxxx" или "P2 7900xxxxxxx",
// P1 79001122332 - в рамках безопасности это может сделать или уже авторизованный номер,  
// P2 79001122333 - или при нажатом тормозе и выключенном двигателе (подразумевается что действие производит владелец изнутри авто) 
// SmsON          - включение уведомления о запуске, остальное смысловое общение, естественно через СМС
// SmsOFF         - выключение уведомления о запуске
// Factory        - сброс на типовые настройки (FactoryReset)
// Time 0..30     - время прогрева от 1 до 30 минут, 0 - автоматический режим
// STms 0..3000   - время прокрутки стартера (в микросекундах) от 1 до 3000 микросекунд, 0 - автоматический режим
// Help           - перечень команд

#include <SoftwareSerial.h>
//#include <MsTimer2.h>
#include <EEPROM.h>
#include <OneWire.h>

// АППАРАТНАЯ КОНФИГУРАЦИЯ (ПОРТЫ)
#define PORT_STARTER 9                       // d9 реле стартера
#define PORT_SECPOWER 10                     // d10 реле цепи вторичного питания печка, фары и т.д.
#define PORT_ENGINE 11                       // d11 реле зажигания
#define PORT_IMMO 12                         // d12 реле обходчика иммобилайзера
//#define PORT_BUZZER 5                        // d5 пищалка (для тестов)
#define PORT_LED_STATUS 13                   // d13 светодиод (для тестов)
#define PORT_MODEM_BOOT 17                   // порт A3 (или 17) управление модемом, инверсия через транзитор
#define PORT_MODEM_TX 2                             // TXd модема = D2
#define PORT_MODEM_RX 3                             // RXd модема = D3 
#define PORT_INPUT_BATTERY A0                       // порт АЦП А0 (или 14) для контроля питания авто
#define PORT_INPUT_DS18B20 A1                       // порт А1 (или 15) для датчика температуры
#define PORT_INPUT_STOP A4                          // порт АЦП A4 (или 18) педаль тормоза
#define PORT_INPUT_ENGINE A5                        // порт АЦП А5 (или 19) для контроля включения зажигания (обратная связь)

// GSM команды, возможно различие между операторами
#define BALANCE_COMMAND "ATD#100#"                  // Команда запроса баланса. Для МТС-МСК выдает в ответ строчку Balance:123.45r
#define COUNT_PHONE 3                               // Максимальное олличество телефонных номеров (от 1 до 16)

#define EWT_MAX 30                                  // Максимальное количество минут прогрева (через СМС меняется от 0 до этого значения)
#define ST_MIN 1000                                 // Минимальное время прокрутки стартера, миллисекунды
#define ST_MAX 3000                                 // Максимальное время прокрутки стартера, миллисекунды

// СИСТЕМНЫЕ ПАРАМЕТРЫ (менять не желательно)
// Конфигурация EEPROM адресов
#define EEPROM_ADDRES_sms_answer 10                 // хранение переменной sms_answer = СМС уведомления о запуске включены 
#define EEPROM_ADDRES_EWT 11                        // хранение переменной int engine_warm_time_setting EWT = время прогрева
#define EEPROM_ADDRES_STARTER 12                    // хранение переменной int starter_time_setting, время работы стартера
#define EEPROM_ADDRES_MASTER0 20                    // masterPhone 0 (начиная с этого адреса храним телефонные номера)
// Константы
const int DELAY_TO_WAITING_2ND_RING = 30000;        // Задержка ожидания 2го звонка для запуска = 30 000 миллисекунд
const int DELAY_BEFORE_DETECTING_START = 6000;      // Столько миллисекунд ждем перед детекцией запуска по напряжению 
// Блок вольтметра (вольтметр для определения факта запуска авто) 
const float VBATT_LOW = 10.0;                       // низкий уровень заряда батареи (аккумулятора) 
const float U1_START = 13.4;                        // Напряжение зарядки АКБ (Двигатель запущен, генератор работает). 
const int MEASURE_COUNT = 10;                       // циклов периода измерения напряжения батареи (среднее значение из N измерений)
                                                    // Формула u1=avarageU1/MEASURE_RATIO*5/1024/R2*(R1+R2), итоговый коэффициент = 5/1024/R2*(R1+R2)=0.022414801
const float MEASURE_RATIO = 0.022415;               // коэффициент (максимально точно измерить для делителя напряжения идущего к аккумулятору авто, 
                                                    // остальные не так важны, главное поставить похожие по номиналам резисторы в делителе
                   
// Настройки, изменяемые посредством СМС 
int engine_warm_time_setting = 10;                  // время работы мотора после запуска 10 минут 
int starter_time_setting = 1000;                    // мс. прокрутки стартера
boolean sms_answer = 0;                             // СМС 0-выключены, 1-включены

// Переменные статуса автозапуска
int engine_warm_time_local = 10;                    // время работы мотора после запуска 10 минут 
int starter_time_local = 1000;                      // мс. прокрутки стартера
int engine_is_started = 0;                          // 0 - двигатель выключен, 1 - двигатель включен 
int left_start_try = 0;                             // переменная для хранения остатка числа попыток запуска 
unsigned long last_start_time = 0;                  // время запуска (когда был запущен) двигатель в миллисекундах 

// Настройки GSM 
SoftwareSerial gsm(PORT_MODEM_TX, PORT_MODEM_RX); 
int ring_counter = 0;                               // счетчик звонков, сбрасывается через время  DELAY_TO_WAITING_2ND_RING (Задержка ожидания 2го звонка) 
unsigned long ring_timer = 0;
int modem_on = 0;                                   // переменная статуса включения модема
char  masterPhone[COUNT_PHONE][12];                 // мастер номера с которых принимаются команды, задаются по СМС, значения здесь не принципиальны
int index_master = 255;                             // индекс номера мастер телефона от которого отправлена команда
int index_master_to_late_answer = 255;              // кому отправляем СМС (индекс массива), если отправка происходит не на текущем шаге программы (Баланс)
String buf = "";
// Датчик температуры
OneWire  ds(PORT_INPUT_DS18B20);                    // on pin 10 (необходим резистор 4,7 кОм)

//==================================================================================================================================================================
void setup() {
  Serial.begin(9600);                                //скорость порта
  //eepromFactoryReset();
  EEPROMReadSettings();
  gsm.begin(9600); //скорость порта модема
  pinMode(PORT_MODEM_BOOT, OUTPUT);           // порт управления загрузкой модема
  
  // Настройка портов   
  pinMode(PORT_STARTER, OUTPUT);                //стартер
  digitalWrite(PORT_STARTER, LOW);              //выкл.
 
  pinMode(PORT_ENGINE, OUTPUT);                 //зажигание
  digitalWrite(PORT_ENGINE, LOW);               //выкл.
 
  pinMode(PORT_SECPOWER, OUTPUT);               //вторичные цепи
  digitalWrite(PORT_SECPOWER, LOW);             //выкл. 

  //pinMode(PORT_BUZZER, OUTPUT);               //пищалка
  //digitalWrite(PORT_BUZZER, LOW);             //выкл.
 
  pinMode(PORT_IMMO, OUTPUT);                   //реле обходчика иммобилайзера
  digitalWrite(PORT_IMMO, LOW);                 //выкл. 

  pinMode(PORT_INPUT_BATTERY, INPUT);                  // порт контроля заряда АКБ
  pinMode(PORT_INPUT_STOP, INPUT);                     // порт контроля педали тормоза
  pinMode(PORT_INPUT_ENGINE, INPUT);                   // порт контроля включенного зажигания для блокировки автозапуска при уже включенном зажигании

SetModem(1);  //Запуск модема
//MsTimer2::set(100, timerInterupt);                // прерывания по таймеру, период 100 мс 
//MsTimer2::start();                                // разрешение прерывания
//left_start_try=3;                                 // ТЕСТ сразу производится запуск
}
//==================================================================================================================================================================
void loop() {
CheckGSMcommand();
if ( DetectStop()) Do_shutdown();                                           // если был нажат тормоз отключаем автозапуск (обеспечение безопасности)
// код программы автозапуска
if (engine_is_started) {                                                    // ждем окончания прогрева
    if (VBatt() < U1_START) Do_shutdown();                                  // проверяем что двиuгатель не заглох
    if (millis() < last_start_time) last_start_time = 0;                    // контроль сброса счетчика millis, милисекунды обнуляются каждые 49 суток
    if (millis() > last_start_time + engine_warm_time_local*60000)          // если текущее время больше чем время старта + время прогрева
        Do_shutdown();   
    }
else { //если двигатель не запущен проверяем возможность запуска - ЭТО есть неиспользованные попытки, генератор не работает, зажигание не включено
  if (!DetectEngine() && VBatt()>VBATT_LOW && left_start_try>0 && VBatt()<U1_START) 
  Do_start();                                                               // запускаем двигатель 
  else Do_shutdown();                                                       // если генератор работает = двигатель запущен, то отменяем автозапуск
  }
}
//========================================================================================================================================================
void timerInterupt() {  // обработка прерывания 1 мс
//if (DetectStop()) flag_stop_pressed = HIGH;// обработка нажатия педали СТОП
}

//==================================================================================================================================================================
/*void SendSMS(String text){ //процедура отправки СМС
if (index_master<255){
  Serial.println("SMS send started to phone index: " + (String)index_master);
  Serial.println("Sended SMS:\n" + (String)text);
  Serial.println("Send to GSMport:" + "AT+CMGS=\"+" + (String)(masterPhone[index_master]) + Strind"\"");
  gsm.print("AT+CMGS=\"+" + (String)(masterPhone[index_master]) + "\"");
  //gsm.print("AT+CMGS=\"+"); 
  //gsm.print((String)(masterPhone[index_master]));
  //gsm.println("\"");
  
  delay22(500);
  gsm.print(text);
  delay22(500);
  gsm.print((char)26);
  delay22(500);
  }
}*/

//===================================================================================

void SendSMS(String text, String phone)  { //процедура отправки СМС
Serial.println("SMS send started to phone: " + phone);
gsm.println("AT+CMGS=\"+" + phone + "\"");
delay22(500);
gsm.print(text);
delay22(500);
gsm.print((char)26);
delay22(500);
Serial.println("Sended SMS:");
Serial.println(text);
}
//==================================================================================================================================================================
void SendSMS(String text1){ //процедура отправки СМС
if (index_master<255) 
  SendSMS(text1, masterPhone[index_master]);
}
//==================================================================================================================================================================
// Функция запуска и осатановки модема
void SetModem(bool stat){                    //1 - включить // 2 выключить
 if (stat){
  digitalWrite(PORT_MODEM_BOOT, LOW);    // установить 1 на входе BOOT модема (0 на контроллере)
  delay22(200);
  digitalWrite(PORT_MODEM_BOOT, HIGH);   // установить 0 на входе BOOT модема (1 на контроллере)
  delay22(2000);
  gsm.println("AT");
  delay22(200);
  gsm.println("ATI");
  delay22(200);
  gsm.println("AT+CLIP=1");                //включаем АОН
  delay22(200);
  gsm.println("AT+CMGF=1");                //режим кодировки СМС - обычный (для англ.)
  delay22(200);
  gsm.println("AT+CSCS=\"GSM\"");          //режим кодировки текста
  delay22(200);
  gsm.println("AT+CNMI=2,2");
  delay22(200);
  Serial.println("modem ON");
  modem_on=1;
 }
 else {
  digitalWrite(PORT_MODEM_BOOT, HIGH);   // установить 0 на входе BOOT модема (1 на контроллере)
  delay22(10);
  gsm.println("AT+CPWROFF");
  Serial.println("modem OFF");
  modem_on = 0; 
 }
}

//==================================================================================================================================================================
void CheckGSMcommand(){ //обработчик звонков и СМС
if (gsm.available()) { //если GSM модуль что-то послал нам, то
  delay22(100); //выждем, чтобы строка успела попасть в порт целиком раньше чем будет считана
  while (gsm.available()) { //сохраняем входную строку в переменную buf
    buf += char(gsm.read());
    delay22(10);
    }
  Serial.println(buf); //печатаем в монитор порта пришедшую строку
  index_master=255;
  for (int i=0; i<COUNT_PHONE; i++) 
    if (buf.indexOf(masterPhone[i])> -1) 
      index_master = i; // присваиваем индекс, если мастер номер
  if (index_master<255 && buf.indexOf("RING")>-1) {  // ЕСЛИ ЗВОНОК С МАСТЕР  ТЕЛЕФОНА 
    gsm.println("ATH0"); //разрываем связь
    ring_timer = millis();
    ring_counter++; 
    }
  // ПРИЕМ КОМАНДЫ УСТАНОВКИ МАСТЕРА ПРИ НАЖАТОМ ТОРМОЗЕ (МОЖНО ПОВЕСИТЬ НА СЛУЖЕБНУЮ КНОПКУ)
  if (index_master<255 || DetectStop() && !DetectEngine())  //если мастер номер ИЛИ служебная кнопка (тормоз) при выключенном зажигании
    ReadAndSetMaster();
  
  // ПРИЕМ и РАЗБОР СМС С АВТОРИЗОВАННОГО ТЕЛЕФОНА, 
  // Можно сразу разбирать команды не определяя СМС это или нет, только для мастера
  if (index_master<255) ReadGSMcommand();

  // отправка Баланса тому кто запросил. СМС с балансом на модуль приходит не от мастера, поэтому обработчик в этой части кода.
  if (buf.indexOf("Balance")>-1) {
    Serial.println(buf.substring (buf.indexOf("Balance"), buf.lastIndexOf("\"")));
    index_master = index_master_to_late_answer;
    SendSMS (buf.substring (buf.indexOf("Balance"), buf.lastIndexOf("\"")));
    index_master_to_late_answer = 255;  
  }
}    
buf = ""; //очищаем буфер

if (ring_counter==1 && millis()-ring_timer>DELAY_TO_WAITING_2ND_RING){ // если был один звонок за период сбросить период измерения
    ring_timer = 0;
    ring_counter = 0;
    if (engine_is_started==1) last_start_time = millis();             // продление времени запуска
    } 
if (ring_counter>1){                                                  // если было 2 звонка за период то запустить
    ring_timer = 0;
    ring_counter = 0;
    //BeepNraz(2); 
    if (left_start_try==0){ //СТАРТ ЗАПУСКА
      left_start_try=3;
      Serial.println("START (2 RINGS)");
      } 
    }
buf = ""; //очищаем 
}
//========================================================================================================================================================
void ReadAndSetMaster(){                                    // УСТАНОВИТЬ МАСТЕР НОМЕР i-ый  
buf.toLowerCase();                                          // преобразовать в нижний регистр 
String tmp="";
for (int i=0; i<COUNT_PHONE; i++) {
tmp="p"+String(i);
if (buf.indexOf(tmp)>-1){                                   // УСТАНОВИТЬ МАСТЕР НОМЕР i-ый, если найдена строка типа "pi"
  GetPhoneFromString(buf, tmp, i);  
  Serial.println("set masterPhone"+String(i)+" = " + (String)masterPhone[i]); 
  for (int a = 0; a < 12; a++)
    EEPROM[EEPROM_ADDRES_MASTER0+i*12 + a] = masterPhone[i][a]; // записываем массив в EEPROM
  }
}
EEPROMReadSettings();   
}
//==================================================================================================================================================================
void GetPhoneFromString (String inp, String stg, int indexphone){ // Из строки inp c позиции строки stg устанавливает телефонный номер 11 знаков в ячейке indexphone
  int start0 = inp.indexOf(stg)+stg.length();
  int start1 = 0;
  int stop1 = 0;
  char ci;
  int i=0;
  for (i=start0; i<inp.length(); i++){            //ищем первую и последнюю цифру
    ci=inp.charAt(i);
    if (ci=='1'||ci=='2'||ci=='3'||ci=='4'||ci=='5'||ci=='6'||ci=='7'||ci=='8'||ci=='9'||ci=='0') {
      if (start1==0) start1=i;
      if (start1>0 && stop1-start1<11) stop1=i; 
      }
  } 
  if (stop1-start1+1==11) {                       // если 11 цифр (2 критерия: цифры и 11 штук), считаем, что это телефонный номер и запоминаем его 
  for (i=start1; i<stop1+1; i++) masterPhone[indexphone][i-start1] = inp.charAt(i);   // копируем 11 ЦИФР  
  masterPhone[indexphone][i-start1]='\0';                                             // последний символ номера = конец строки  
  }
}
//========================================================================================================================================================
void ReadGSMcommand(){                                      //обработчик команд из СМС
String tmp="";
int tt=0;  
buf.toLowerCase();                                          // преобразовать в нижний регистр 
  if (buf.indexOf("start")>-1) {                            // ЗАПУСТИТЬ ДВИГАТЕЛЬ
    if (left_start_try==0) left_start_try=3;                // указываем, что надо запускаться? 3 попытки
    Serial.println("start command"); 
    }
  else if (buf.indexOf("stop")>-1){                         //ОСТАНОВИТЬ ДВИГАТЕЛЬ
    Serial.println("stop command");
    Do_shutdown();
    delay22(500);
    if (sms_answer) SendSMS(Info());
    } 
  else if (buf.indexOf("list")>-1){                         //отправить по СМС все мастер телефоны
    tmp="Phones: \n"; 
    for (int i=0; i<COUNT_PHONE; i++) {
      tmp+=masterPhone[i]; 
      tmp+="\n";
      }
    SendSMS (tmp);
    Serial.println(tmp);
    }
  else if (buf.indexOf("info")>-1){                          // вывод статуса
    tmp = Info();
    SendSMS (tmp);   
    Serial.println("info SMS:");                              // вывод статуса и напряжений
    Serial.println(tmp);                                     // вывод статуса и напряжений
    }
  else if (buf.indexOf("smson")>-1){
    Serial.println("SMS ON command");                        // включить ответные СМС
    sms_answer = 1;
    EEPROM.update(EEPROM_ADDRES_sms_answer, (bool)sms_answer);           // записали число по адресу 10
    SendSMS ("SMS ON - ok");   
    }
  else if (buf.indexOf("smsoff")>-1){
    Serial.println("SMS OFF command");                       // выключить ответные СМС
    sms_answer = 0;
    EEPROM.update(EEPROM_ADDRES_sms_answer, (bool)sms_answer);           // записали число по адресу 10
    SendSMS ("SMS OFF - ok");   
    }
  else if (buf.indexOf("factory")>-1){                  // перезагрузка данных
    Serial.println("factory reset command");
    EEPROM.update(0, 255); 
    EEPROMFactoryReset();  
    delay22(100);
    EEPROMReadSettings();
    delay22(100);
    SendSMS ("Factory Reset - ok");   
    }
  else if (buf.indexOf("time")>-1){                           // время работы двигателя в минутах
    engine_warm_time_setting = constrain(GetIntFromBuf("time"), 0, EWT_MAX);
    EEPROM.update(EEPROM_ADDRES_EWT, engine_warm_time_setting);  // сколько времение в минутах прогревать двигатель  
    Serial.println("set engine_warm_time_setting = " + (String)(engine_warm_time_setting)); 
    if (engine_warm_time_setting>0) 
      SendSMS ("Engine warm time = " + (String)(engine_warm_time_setting));
      else 
      SendSMS ("Engine warm Time = AUTO");
    }
  else if (buf.indexOf("stms")>-1){                           // STMS = STARTER TIME MILLISECONDS = время работы стартера в миллисекундах  
    starter_time_setting = constrain(GetIntFromBuf("stms"), 0, ST_MAX);
    byte raw[2];                                              // запись времени прокрутки стартера starter_time_setting=
    (int&)raw = starter_time_setting;
    for(byte i = 0; i < 2; i++) EEPROM.write(EEPROM_ADDRES_STARTER+i, raw[i]);
    Serial.println("set starter_time_setting = "+String(starter_time_setting)); 
    SendSMS ("Starter time = " + (String)(starter_time_setting) + "ms");
    }
  else if (buf.indexOf("help")>-1){
    tmp="All commands:\nStart\nStop\nBalance\nInfo\nList\nP0.." + String(COUNT_PHONE-1) + " phone\nSmsON\nSmsOFF\nFactory\n";
    tmp+="Time 0.." + String(EWT_MAX) + "\nHelp";
    Serial.println(tmp);  
    SendSMS (tmp); 
    }
  else if (buf.indexOf("balance")>-1){
    Serial.println("Вalance command");  
    gsm.println(BALANCE_COMMAND); 
    index_master_to_late_answer = index_master;
    }
buf = ""; //очищаем
}
//========================================================================================================================================================
int GetIntFromBuf(String startStr){ // получает целочисленное из строки после слова - параметра функции startStr
startStr = buf.substring(buf.indexOf(startStr)+startStr.length(), buf.lastIndexOf('\0'));
return startStr.toInt();
}
//========================================================================================================================================================
float GetFloatFromBuf(String startStr){ // получает число с плавающей точкой из строки после слова - параметра функции startStr
float floatVar;
char floatbufVar[32];
String stringVar = buf.substring(buf.indexOf(startStr)+startStr.length(), buf.lastIndexOf('\0'));
stringVar.toCharArray(floatbufVar,sizeof(floatbufVar));
floatVar=atof(floatbufVar);
return floatVar;
}   // !!!!!!!!!!!!!!!!  проверить функцию 
//========================================================================================================================================================
String Info() // функция собирает информационную строку для СМС
{
float ttemp = Temp();  
String strtmp = "";
if (engine_is_started==0) strtmp = strtmp + "Not Running\n";                  //"Engine is NOT Running\n"; 
  else strtmp = strtmp + "Is Running\n";                   //"Engine Is Running\n";
strtmp = strtmp + "Batt: "+String(VBatt())+"V\n";     //"Voltage: "+String(VBatt())+"V\n";
strtmp = strtmp + "WarmTime: ";                       //"EngineWarmTime: "; 
if (engine_warm_time_setting>0) strtmp = strtmp + (String)(engine_warm_time_setting)+"min." + '\n'; 
  else strtmp = strtmp + "auto\n";
if (engine_is_started==1) strtmp = strtmp + "Time Left: " + (String)(engine_warm_time_local-(millis()-last_start_time)/60000) + "min.\n"; 
if (ttemp>-100)                                // если датчика температуры нет, то функция возврящает -100 и значение не выводится
  strtmp = strtmp + "Temp: " + (String)(ttemp)+"C\n";                             
if (sms_answer) 
  strtmp = strtmp + "SMS ON\n"; 
  else 
  strtmp = strtmp + "SMS OFF\n";
strtmp = strtmp + "UpTime: " + String(millis()/3600000) + "Hr";
return strtmp;
}
//========================================================================================================================================================
void Do_start() {// запуск двигателя  
Serial.println("function Do_start"); 
Serial.print("left_start_try = ");         Serial.println(left_start_try);
digitalWrite(PORT_IMMO, HIGH);      Serial.println("IMMO ON");         //включаем обходчик иммобилайзера
delay22(100);
digitalWrite(PORT_ENGINE, HIGH);    Serial.println("ENGINE ON");       //включаем зажигание 
delay22(500);
digitalWrite(PORT_SECPOWER, HIGH);  Serial.println("SECPOWER ON");     // включаем печку фары и тд
delay22(3000);                                                                // ждем, чтобы бензонасос набрал давление
digitalWrite(PORT_SECPOWER, LOW);   Serial.println("SECPOWER OFF");    // выключаем печку фары и тд.
delay22(500);                              Serial.println("delay(500)");
if (starter_time_setting>0) starter_time_local = starter_time_setting;        // устанавливаем переменную подсчета времени прогрева
  else {                                                                      // здесь устанавливаем автоматическое значение по темпаратуре
    starter_time_local = map(Temp(), 30, -25, ST_MIN, ST_MAX);                // автоподбор значения стартера
    starter_time_local = constrain(starter_time_local, ST_MIN, ST_MAX);       // ограничиваем время работы стартера  
  }
digitalWrite(PORT_STARTER, HIGH);   Serial.println("STARTER ON");      // включаем стартер 
delay22(starter_time_local+300*(3-left_start_try));                           // продолжаем его держать включенным время starter_time_local
                                           Serial.println("delay " + String (starter_time_local+200*(3-left_start_try)));
digitalWrite(PORT_STARTER, LOW);    Serial.println("STARTER OFF");     // отключаем стартер 
delay22(DELAY_BEFORE_DETECTING_START);     Serial.println("delay "+String(DELAY_BEFORE_DETECTING_START)); 
if (VBatt()>U1_START) {                                                       // смотрим, что  зарядка пошла  
  last_start_time = millis();                                                 // запоминаем время запуска движка
  digitalWrite(PORT_SECPOWER, HIGH); Serial.println("SECPOWER ON");    //включаем печку фары итд
  engine_is_started = 1;                                                      //Запоминаем что движок запущен
  left_start_try = 0;                                                         // завелся, дальше не пытаемся
  if (engine_warm_time_setting>0) engine_warm_time_local = engine_warm_time_setting;          // устанавливаем переменную подсчета времени прогрева
  else {                                                                      // здесь устанавливаем автоматическое значение по темпаратуре
    engine_warm_time_local = map(Temp(), 30, -25, 5, 30);                     // автоподбор значения прогрева
    engine_warm_time_local = constrain(engine_warm_time_local, 5, EWT_MAX);   // ограничиваем таймер в зачениях от 5 до EWT_MAX минут 
  }
  Serial.println("STARTED\n"+Info());
  if (sms_answer==1) SendSMS(Info());
  }
else{ // не завелся
  digitalWrite(PORT_SECPOWER, LOW);   Serial.println("SECPOWER OFF");  //выключаем печку фары и тд
  delay22(500);                              Serial.println("delay(500)");
  digitalWrite(PORT_ENGINE, LOW);     Serial.println("ENGINE OFF");    //выключаем зажигание чтобы разблокировать реле стартера
  digitalWrite(PORT_IMMO, LOW);       Serial.println("IMMO OFF");      //выключаем обходчик иммобилайзера
  engine_is_started = 0;                                                      //Запоминаем что движок не запущен
  Serial.println("NOT STARTED\n"+Info());
  if (sms_answer==1 && left_start_try==1) SendSMS(Info());                        // отправка СМС о неудачном запуске
  if (left_start_try>1) delay22(DELAY_BEFORE_DETECTING_START);                // пауза между запусками
  }
if (left_start_try>0) left_start_try--;                                       // уменьшаем число попыток на одну состоявшуюся
}
//========================================================================================================================================================
void Do_shutdown (){
if (digitalRead(PORT_SECPOWER)){
  digitalWrite(PORT_SECPOWER, LOW);     //выключаем печку фары и т.д. 
  Serial.println("SECPOWER LOW");   
  delay22(100);  
  }
if (digitalRead(PORT_ENGINE)){
  digitalWrite(PORT_ENGINE, LOW);       //выключаем зажигание
  Serial.println("ENGINE LOW");
  }
left_start_try = 0;
engine_is_started = 0;                  // двигатель выключили 
last_start_time = 0;                    // обнулили время включения
}
//=======================================================================================================================================================
boolean DetectEngine(){// 1 если включено зажигание ключом
if (analogRead(PORT_INPUT_ENGINE)*MEASURE_RATIO > VBATT_LOW) 
  return 1;  
else 
  return 0;  
}
//=======================================================================================================================================================
boolean DetectStop(){
if (analogRead(PORT_INPUT_STOP)*MEASURE_RATIO>VBATT_LOW) 
  return 1; // если нажат стоп
else 
  return 0;  
}
//========================================================================================================================================================
void delay22(unsigned long c){  //  функция задержки, не блокирует прерывания
  unsigned long end_time = millis() + c;
  while ( end_time > millis() ) c = end_time; // сама операция не принципиальна, но без нее компилятор не корректно обрабатывает данную функцию
}
//========================================================================================================================================================
float VBatt() {      // блок замера напряжения с усреднением по выборке MEASURE_COUNT значений (избавляет от случайных скачков в замерах)
long sumU1 = 0;       // переменные для суммирования кодов АЦП
float utemp1 = 0;     // измеренное напряжение
for (int timeCount=0; timeCount < MEASURE_COUNT; timeCount++) sumU1+= analogRead(PORT_INPUT_BATTERY); // суммирование кодов АЦП
utemp1 = sumU1 / MEASURE_COUNT * MEASURE_RATIO;   
return utemp1;
}   
//========================================================================================================================================================
float Temp () { //получение температуры с датчика
 byte i; 
 byte type_s; 
 byte data[12]; 
 byte addr[8]; 
 float celsius; 
 int cnt=3;
 while (!ds.search(addr) && cnt>0) {
     ds.reset_search(); 
     cnt--;
     } 
 if (cnt==0) return -100; //3 попытки определения адреса датчика
 if (OneWire::crc8(addr, 7) != addr[7]) return -100;// CRC is not valid!
 switch (addr[0]) {
    case 0x10: 
        type_s = 1; 
        break; 
    case 0x28: 
        type_s = 0; 
        break; 
    case 0x22: 
        type_s = 0; 
        break; 
    default: 
        return -100; 
    } 
 ds.reset(); 
 ds.select(addr); 
 ds.write(0x44); 
 //ds.write(0x44, 1); delay(1000); // для паразитного питания датчика
 ds.reset(); 
 ds.select(addr); 
 ds.write(0xBE); 
 for ( i = 0; i < 9; i++) 
    data[i] = ds.read(); 
    int16_t raw = (data[1] << 8) | data[0];
 if(type_s){
     raw=raw<<3;
    if(data[7]==0x10) 
        raw=(raw&0xFFF0)+12-data[6];
    }
 else{
     byte cfg=(data[4]&0x60);
     if(cfg==0x00) raw=raw&~7;
     else 
        if(cfg==0x20) raw=raw&~3;
            else 
            if(cfg==0x40)raw=raw&~1;
    }
 celsius = (float)raw / 16.0; 
 return celsius;
}
//==================================================================================================================================================================
/*void BeepNraz(int n){ //Зуммер сигналит N раз
  for (int i=0; i<n; i++){
  tone (PORT_BUZZER, 1000, 100); 
  delay22(300); 
  }
}*/
//========================================================================================================================================================
void EEPROMFactoryReset(){
EEPROM.update(EEPROM_ADDRES_sms_answer, 0);    // sms_answer=0 записали число 0 по адресу 10
EEPROM.update(EEPROM_ADDRES_EWT, 10);          // engine_warm_time_setting=10 записали по адресу 11
byte raw[2];                                // запись времени прокрутки стартера starter_time_setting=1000
(int&)raw = 1000;
for(byte i = 0; i < 2; i++) EEPROM.write(EEPROM_ADDRES_STARTER+i, raw[i]);
// стирание телефонных номеров
for (int i=0; i<COUNT_PHONE; i++) EEPROM.update(EEPROM_ADDRES_MASTER0+i*12, "");
}
//========================================================================================================================================================
void EEPROMReadSettings(){
// включены ли СМС  
EEPROM.get(EEPROM_ADDRES_sms_answer, sms_answer);   
sms_answer = constrain(sms_answer, 0, 1);
Serial.println("sms_answer = " + (String)(sms_answer));

// чтение времени в минутах сколько прогревать двигатель  
EEPROM.get(EEPROM_ADDRES_EWT, engine_warm_time_setting);                              
engine_warm_time_setting = constrain(engine_warm_time_setting, 0, EWT_MAX);           // 0 = авто определение по температуре
Serial.println("engine_warm_time_setting = " + (String)(engine_warm_time_setting)); 

// чтение времени прокрутки стартера starter_time_setting
byte raw[2];
for(byte i = 0; i < 2; i++) raw[i] = EEPROM.read(EEPROM_ADDRES_STARTER+i); 
int &num = (int&)raw;
starter_time_setting = constrain(num,0, ST_MAX);
Serial.println("starter_time_setting = "+String(starter_time_setting)); 

// чтение номеров 
for (int i=0; i<COUNT_PHONE; i++) {                                                  
  EEPROM.get(EEPROM_ADDRES_MASTER0+i*12, masterPhone[i]);                      
  masterPhone[i][11]='\0';
  Serial.println("masterPhone[" + String(i) + "] = " + (String)masterPhone[i]);                      //вывести все мастер телефоны
  }
}
