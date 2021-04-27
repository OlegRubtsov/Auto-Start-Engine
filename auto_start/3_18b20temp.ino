float Temp () { //получение температуры с датчика
 byte i; 
 byte type_s; 
 byte data[12]; 
 byte addr[8]; 
 float celsius; 
 int cnt=3;
 while (!ds.search(addr) && cnt>0) {
     ds.reset_search(); 
     cnt--;
     } 
 if (cnt==0) return -100; //3 попытки определения адреса датчика
 if (OneWire::crc8(addr, 7) != addr[7]) return -100;// CRC is not valid!
 switch (addr[0]) {
    case 0x10: 
        type_s = 1; 
        break; 
    case 0x28: 
        type_s = 0; 
        break; 
    case 0x22: 
        type_s = 0; 
        break; 
    default: 
        return -100; 
    } 
 ds.reset(); 
 ds.select(addr); 
 ds.write(0x44); 
 //ds.write(0x44, 1); delay(1000); // для паразитного питания датчика
 ds.reset(); 
 ds.select(addr); 
 ds.write(0xBE); 
 for ( i = 0; i < 9; i++) 
    data[i] = ds.read(); 
    int16_t raw = (data[1] << 8) | data[0];
 if(type_s){
     raw=raw<<3;
    if(data[7]==0x10) 
        raw=(raw&0xFFF0)+12-data[6];
    }
 else{
     byte cfg=(data[4]&0x60);
     if(cfg==0x00) raw=raw&~7;
     else 
        if(cfg==0x20) raw=raw&~3;
            else 
            if(cfg==0x40)raw=raw&~1;
    }
 celsius = (float)raw / 16.0; 
 return celsius;
}
