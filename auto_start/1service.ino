//========================================================================================================================================================
int GetIntFromBuf(String startStr){ // получает целочисленное из строки после слова - параметра функции startStr
startStr = buf.substring(buf.indexOf(startStr)+startStr.length(), buf.lastIndexOf('\0'));
return startStr.toInt();
}
float GetFloatFromBuf(String startStr){ // получает число с плавающей точкой из строки после слова - параметра функции startStr
float floatVar;
char floatbufVar[32];
String stringVar = buf.substring(buf.indexOf(startStr)+startStr.length(), buf.lastIndexOf('\0'));
stringVar.toCharArray(floatbufVar,sizeof(floatbufVar));
floatVar=atof(floatbufVar);
return floatVar;
}   // !!!!!!!!!!!!!!!!  проверить функцию 
//========================================================================================================================================================
void delay22(unsigned long c){  //  функция задержки, не блокирует прерывания
  unsigned long end_time = millis() + c;
  while ( end_time > millis() ) c = end_time; // сама операция не принципиальна, но без нее компилятор не корректно обрабатывает данную функцию
}
 
