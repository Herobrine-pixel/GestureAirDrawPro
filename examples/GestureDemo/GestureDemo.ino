#include <GestureAirDrawPro.h>
GestureAirDrawPro gad;

void setup(){
  Serial.begin(115200);
  while(!Serial){}
  Serial.println("GestureAirDrawPro demo - MPU6050 + UNO");
  gad.begin(0x68, 2);
  Serial.println("Hold button to draw; release to stop.");
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
