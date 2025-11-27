# GestureAirDrawPro

GestureAirDrawPro is a ready-to-use Arduino library for **air-drawn gesture recognition** using an **MPU6050** IMU on an **Arduino UNO**. It records orientation (pitch & roll), smooths the signal, matches gestures against built-in templates (letters A–Z and digits 0–9) using **Dynamic Time Warping (DTW)**, and exports the recognized label plus an SVG polyline of the drawing.

## Features
- MPU6050 over I2C (Wire)
- Complementary filter for stable orientation (pitch, roll)
- Start/stop recording with a button (D2 by default) or programmatic control
- DTW-based matching for letters (A–Z) and digits (0–9) + basic shapes (circle, square, triangle, line, zigzag, check)
- Export SVG via Serial
- Example for Arduino IDE and PlatformIO
- `.gitignore`, `library.json`, `library.properties` included for GitHub/Library Manager readiness

## Wiring (Arduino UNO)

MPU6050 typical wiring to Arduino UNO:

- **VCC** -> **5V** (or 3.3V if your module requires it)
- **GND** -> **GND**
- **SDA** -> **A4**
- **SCL** -> **A5**
- **INT** (optional) -> not required
- **Button (optional)** -> **D2** (connect to GND when pressed; library uses `INPUT_PULLUP`)

If your MPU6050 AD0 is HIGH, the I2C address is `0x69` instead of `0x68`.

## Quickstart
1. Connect MPU6050 to Arduino UNO as above.
2. Open `examples/GestureDemo/GestureDemo.ino` in Arduino IDE.
3. Upload and open Serial Monitor at **115200** baud.
4. Press and hold the button to start drawing; release to stop. After stopping the library prints the recognized label and an SVG polyline string.

## API (short)
```cpp
#include <GestureAirDrawPro.h>
GestureAirDrawPro gad;

void setup(){
  Serial.begin(115200);
  gad.begin(); // gad.begin(i2c_addr = 0x68, buttonPin = 2);
}

void loop(){
  gad.update(); // call frequently
  if(gad.availableResult()){
    Serial.println(gad.getResultLabel());
    gad.printSVG(Serial);
  }
}
```

## Notes & Tips
- Attach the IMU rigidly to the drawing tool (pen, stick) for consistent tracking.
- Calibrate neutral pose with `gad.calibrate()` while holding still.
- For best letter/digit recognition, draw with consistent stroke direction and speed; DTW is tolerant but works better with similar stroke order.
- Templates can be added or tuned in `src/GestureAirDrawPro.cpp` generator (no large stored tables to keep library small).
