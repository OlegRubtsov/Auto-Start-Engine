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

if (ring_counter==1 && millis()-ring_timer>DELAY_TO_WAITING_2ND_RING){ // если был только один звонок за период сбросить период измерения
    ring_timer = 0;
    ring_counter = 0;
    if (engine_is_started==1) last_start_time = millis();             // продление времени запуска
    } 
if (ring_counter>1){                                                  // если было 2 звонка за период то запустить
    ring_timer = 0;
    ring_counter = 0;
    //BeepNraz(2); 
    if (left_start_try==0){                                           //СТАРТ ЗАПУСКА
      left_start_try=3;
      Serial.println("Start (2 RINGS)");
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
if (buf.indexOf(tmp)>-1){                                   // УСТАНОВИТЬ МАСТЕР НОМЕР i-ый, если найдена строка типа "p[индекс] [номер телефона]"
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
