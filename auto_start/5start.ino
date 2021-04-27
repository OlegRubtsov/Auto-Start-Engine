void Do_start() {// запуск двигателя  
Serial.println("function Do_start"); 
Serial.print("left_start_try = ");         Serial.println(left_start_try);
digitalWrite(PORT_IMMO, HIGH);      Serial.println("IMMO ON");         // включаем обходчик иммобилайзера
delay22(100);
digitalWrite(PORT_ENGINE, HIGH);    Serial.println("ENGINE ON");       // включаем зажигание 
delay22(500);
if (!DetectHandBrake()){                                               // прверяем для безопасности активацию ручника, 
                                                                       // для АКПП вход ручника подключить к питанию (+12В)
  Do_shutdown();
  return;
}
digitalWrite(PORT_SECONDPOWER, HIGH);  Serial.println("SECPOWER ON");     // включаем печку фары и тд
delay22(3000);                                                                // ждем, чтобы бензонасос набрал давление
digitalWrite(PORT_SECONDPOWER, LOW);   Serial.println("SECPOWER OFF");    // выключаем печку фары и тд.
delay22(500);                              Serial.println("delay(500)");
if (starter_time_setting>0) starter_time_local = starter_time_setting;        // устанавливаем переменную подсчета времени прогрева
  else {                                                                      // здесь устанавливаем автоматическое значение по темпаратуре
    starter_time_local = map(Temp(), 30, -25, ST_MIN, ST_MAX);                // автоподбор значения стартера
    starter_time_local = constrain(starter_time_local, ST_MIN, ST_MAX);       // ограничиваем время работы стартера  
  }
digitalWrite(PORT_STARTER, HIGH);                                             // включаем стартер
Serial.println("STARTER ON"); 
      
delay22(starter_time_local+300*(3-left_start_try));                           // продолжаем его держать включенным время starter_time_local
Serial.println("delay " + String (starter_time_local+200*(3-left_start_try)));

digitalWrite(PORT_STARTER, LOW);                                              // отключаем стартер
Serial.println("STARTER OFF");                                                
 
delay22(DELAY_BEFORE_DETECTING_START);                                        // задержка перед замером уровня напряжения (перед детекцией старта) 
Serial.println("delay "+String(DELAY_BEFORE_DETECTING_START)); 

if (VBatt()>U1_START) {                                                       // смотрим, что  зарядка пошла  
  last_start_time = millis();                                                 // запоминаем время запуска движка
  digitalWrite(PORT_SECONDPOWER, HIGH);                                          //включаем печку фары итд
  Serial.println("SECONDPOWER ON");           
  engine_is_started = 1;                                                      //Запоминаем что движок запущен
  left_start_try = 0;                                                         // завелся, дальше не пытаемся
  if (engine_warm_time_setting>0) 
    engine_warm_time_local = engine_warm_time_setting;                        // устанавливаем переменную подсчета времени прогрева
  else {                                                                      // здесь устанавливаем автоматическое значение по темпаратуре
    engine_warm_time_local = map(Temp(), 30, -25, 5, 30);                     // автоподбор значения прогрева
    engine_warm_time_local = constrain(engine_warm_time_local, 5, EWT_MAX);   // ограничиваем таймер в зачениях от 5 до EWT_MAX минут 
  }
  Serial.println("STARTED\n"+Info());
  if (sms_answer==1) SendSMS(Info());
  }
else{ // не завелся
  digitalWrite(PORT_SECONDPOWER, LOW);                                    //выключаем печку фары и тд
  Serial.println("SECONDPOWER OFF");  
  delay22(500);                       
  Serial.println("delay(500)");
  digitalWrite(PORT_ENGINE, LOW);                                      //выключаем зажигание чтобы разблокировать реле стартера 
  Serial.println("ENGINE OFF");    
  engine_is_started = 0;                                               //Запоминаем что движок не запущен
  Serial.println("NOT STARTED\n"+Info());
  if (sms_answer==1 && left_start_try==1) 
    SendSMS(Info());                                                   // отправка СМС о неудачном запуске
  if (left_start_try>1) 
    delay22(DELAY_BEFORE_DETECTING_START);                             // пауза между запусками
  }

if (digitalRead(PORT_IMMO)){
  digitalWrite(PORT_IMMO, LOW);                                          // выключаем обходчик иммобилайзера
  Serial.println("IMMO OFF"); 
}
if (left_start_try>0) left_start_try--;                                // уменьшаем число попыток на одну состоявшуюся
}
