#ifndef GESTURE_AIR_DRAW_PRO_H
#define GESTURE_AIR_DRAW_PRO_H

#include <Arduino.h>
#include <Wire.h>

#define MAX_SAMPLES 300
#define TEMPLATE_POINTS 48
#define NUM_TEMPLATES 36 // 26 letters + 10 digits

struct Point { float x; float y; };

class GestureAirDrawPro {
public:
  GestureAirDrawPro();
  bool begin(uint8_t i2c_addr = 0x68, uint8_t buttonPin = 2);
  void update(); // call in loop()
  void calibrate();
  void startRecording();
  void stopRecording();
  bool availableResult() const;
  const char* getResultLabel() const;
  void printSVG(Print &out) const;
  void exportSVGToFile(Print &out) const;
  void setSampleMs(unsigned long ms);
  void setAlpha(float alpha);
private:
  TwoWire* _wire;
  uint8_t _addr;
  float _pitch, _roll;
  float _alpha;
  unsigned long _lastMs;
  unsigned long _sampleMs;
  Point _buf[MAX_SAMPLES];
  int _count;
  bool _recording;
  char _resultLabel[32];
  bool _hasResult;
  uint8_t _buttonPin;
  // internal helpers
  void readRawAccelGyro(float &ax,float &ay,float &az,float &gx,float &gy,float &gz);
  void mapOrientationToXY(float &x,float &y);
  void normalizeToUnit(Point *arr, int n);
  void resample(const Point *src, int nsrc, Point *dst, int ndst);
  float dtwDistance(const Point *a, int na, const Point *b, int nb);
  void fillTemplate(Point *out, int n, int id);
};

#endif
