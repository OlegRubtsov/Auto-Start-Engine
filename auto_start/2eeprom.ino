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
