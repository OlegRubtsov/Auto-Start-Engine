#define PORT_OUTPUT_IMMO 12                         // d12 обходчик иммобилайзера

void setup() {
Serial.begin(9600);                                //скорость порта

pinMode(PORT_OUTPUT_IMMO, OUTPUT);                   //реле обходчика иммобилайзера
digitalWrite(PORT_OUTPUT_IMMO, LOW);                 //выкл. 

}

void loop() {
digitalWrite(PORT_OUTPUT_IMMO, HIGH);      
Serial.println("IMMO ON");         //включаем обходчик иммобилайзера
delay(20000);
digitalWrite(PORT_OUTPUT_IMMO, LOW);      
Serial.println("IMMO OFF");         //выключаем обходчик иммобилайзера
delay(20000);
}
