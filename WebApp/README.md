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

## Release notes & description
**Release name:** GestureAirDrawPro v1.0.0  
**Description:** A minimal, ready-to-run web frontend for testing and visualizing gesture JSON data from Arduino + MPU6050. Includes simulator and sample wiring diagram.

## License
MIT

