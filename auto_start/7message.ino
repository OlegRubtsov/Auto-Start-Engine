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
