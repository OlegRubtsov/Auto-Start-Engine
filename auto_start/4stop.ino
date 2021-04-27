void Do_shutdown (){
//if (digitalRead(PORT_SECONDPOWER)){
  digitalWrite(PORT_SECONDPOWER, LOW);     //выключаем печку фары и т.д. 
  Serial.println("SECPOWER LOW");   
  delay22(100);  
//  }
//if (digitalRead(PORT_ENGINE)){
  digitalWrite(PORT_ENGINE, LOW);       //выключаем зажигание
  Serial.println("ENGINE LOW");
//  }
//if (digitalRead(PORT_IMMO)){
  digitalWrite(PORT_IMMO, LOW);                                          // выключаем обходчик иммобилайзера
  Serial.println("IMMO OFF"); 
//}
left_start_try = 0;
engine_is_started = 0;                  // двигатель выключили 
last_start_time = 0;                    // обнулили время включения
}
