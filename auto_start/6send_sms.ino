
void SendSMS2(String text, String phone)  { //процедура отправки СМС
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
  SendSMS2(text1, masterPhone[index_master]);
}
