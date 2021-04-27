#include <SoftwareSerial.h>
#define PORT_MODEM_BOOT 17                          // порт A3 (или 17) управление модемом, инверсия через транзитор
#define PORT_MODEM_TX 2                             // TXd модема = D2
#define PORT_MODEM_RX 3                             // RXd модема = D3 

// Настройки GSM 
SoftwareSerial gsm(PORT_MODEM_TX, PORT_MODEM_RX);  // те же порты что и в основном скетче

byte led = 13;

void setup() 
{
  delay(2000);  
  pinMode(led, OUTPUT);
  digitalWrite(led, LOW);
  Serial.begin(9600);  
  gsm.begin(9600); // скорость порта модема, именно ее может понадобиться подобрать.
  Serial.println("GSM NEOWAY M590 TEST");
  digitalWrite(PORT_MODEM_BOOT, LOW);    // установить 1 на входе BOOT модема (0 на контроллере)
  delay(200);
  digitalWrite(PORT_MODEM_BOOT, HIGH);   // установить 0 на входе BOOT модема (1 на контроллере)
  delay(2000);
  gsm.println("AT");
  delay(200);
  gsm.println("ATI");
  delay(200);
  gsm.println("AT+CLIP=1");                //включаем АОН
  delay(200);
  gsm.println("AT+CMGF=1");                //режим кодировки СМС - обычный (для англ.)
  delay(200);
  gsm.println("AT+CSCS=\"GSM\"");          //режим кодировки текста
  delay(200);
  gsm.println("AT+CNMI=2,2");
  delay(200);
  Serial.println("modem is ON");
}

void loop() 
{ 
 if(Serial.available()) //если в мониторе порта ввели что-то
  {  
    char ch = ' ';
    String val = "";
    
    while (Serial.available()) 
     {  
       ch = Serial.read();
       val += char(ch); //собираем принятые символы в строку
       delay(5);
     }

    if(val.indexOf("callme") > -1) // своя команда, усли написать в терминале callme модем перезвонит на указанный номер
     {  
       gsm.println("ATD89111111111;"); // ВПИШИТЕ номер, на который нужно позвонить
     }

    else gsm.println(val);  // передача всего, что набрано в терминале в GSM модуль
  }


 while(gsm.available()) 
  { 
    Serial.print((char)gsm.read());
    delay(5);
  }
}
