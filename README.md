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

## License
MIT License

Copyright (c) 2025

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.

# GestureAirDrawPro - WebApp

Release: **v1.0.0 — GestureAirDrawPro - WebApp**

A single-file WebApp frontend that connects to an Arduino UNO (with an MPU6050) via the **Web Serial API**, receives ultra-detailed JSON gesture packets, draws the gesture path, shows extracted features and alternatives, and renders a simple 3D orientation preview.

## Included files
- `index.html` — main UI
- `style.css` — styling
- `script.js` — app logic (Web Serial, JSON parsing, simulator)
- `README.md` — this file

## JSON format
The app expects JSON messages similar to:

```json
{
  "input": {
    "raw": [
      {"ax":0.12,"ay":0.05,"az":0.98,"gx":-3.2,"gy":1.8,"gz":0.5,"t":0}
    ],
    "normalized": [
      {"x":12.1,"y":3.4,"t":0}
    ]
  },
  "features": {
    "angles": [32.4, 58.9, 89.1],
    "segments": [
      {"dx":12,"dy":3,"len":12.36},
      {"dx":-4,"dy":8,"len":8.94}
    ],
    "length": 151.4,
    "dtw_distance": 12.8
  },
  "result": {
    "type": "shape",
    "name": "Triangle",
    "confidence": 0.92,
    "alternatives": [
      {"name":"A","confidence":0.55},
      {"name":"V","confidence":0.44}
    ]
  }
}
```

### Notes for Arduino (UNO) + MPU6050
- Connect MPU6050 I2C: `A4 -> SDA`, `A5 -> SCL`, `VCC -> 5V (or 3.3V)`, `GND -> GND`.
- Using the MPU6050 you can compute accelerometer and gyro values and package them into the JSON format above.
- Send complete JSON objects terminated by a newline. For stability send `}\n` after each JSON object to help the parser in this WebApp detect boundaries.

### Example Arduino approach
- Use `Wire` + `MPU6050` or `I2Cdev` libraries to read sensor values.
- Build normalized coordinates (scale/center them) and send the JSON string with `Serial.println(jsonString);` at `115200` baud.

### How to use
1. Open `index.html` in a Chromium-based browser (Chrome/Edge) supporting Web Serial.
2. Click **Connect Web Serial** and select your Arduino port.
3. Use simulator buttons to test: Triangle / Circle / Letter A.
4. The app parses the JSON, draws the normalized path, shows features, and rotates the 3D cube to reflect pitch/roll.


## Developer
### Herobrine Pixel
### 2025

#### Dont forget to give a star..

