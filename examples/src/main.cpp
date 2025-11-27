#include <Arduino.h>
#include <GestureAirDrawPro.h>
GestureAirDrawPro gad;
void setup(){
  Serial.begin(115200);
  gad.begin(0x68, 2);
  Serial.println("GestureAirDrawPro (PlatformIO) ready");
}
void loop(){
  gad.update();
  if(gad.availableResult()){
    Serial.print("Recognized: ");
    Serial.println(gad.getResultLabel());
    gad.printSVG(Serial);
    delay(500);
  }
}
