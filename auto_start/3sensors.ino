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
//=======================================================================================================================================================
boolean DetectHandBrake(){
if (analogRead(PORT_INPUT_HANDBRAKE)*MEASURE_RATIO>VBATT_LOW) 
  return 1; // если ручник активен
else 
  return 0;  
}
//========================================================================================================================================================
float VBatt() {      // блок замера напряжения с усреднением по выборке MEASURE_COUNT значений (избавляет от случайных скачков в замерах)
long sumU1 = 0;       // переменные для суммирования кодов АЦП
float utemp1 = 0;     // измеренное напряжение
for (int timeCount=0; timeCount < MEASURE_COUNT; timeCount++) sumU1+= analogRead(PORT_INPUT_BATTERY); // суммирование кодов АЦП
utemp1 = sumU1 / MEASURE_COUNT * MEASURE_RATIO;   
return utemp1;
}  
