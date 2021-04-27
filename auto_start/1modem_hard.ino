// Функция запуска и осатановки модема
void SetModem(bool stat){                    //1-включить // 0-выключить
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
