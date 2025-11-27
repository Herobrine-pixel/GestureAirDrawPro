#include "GestureAirDrawPro.h"
#include <Wire.h>
#include <math.h>

#define RAD_TO_DEG 57.29577951308232f

GestureAirDrawPro::GestureAirDrawPro(){
  _wire = &Wire;
  _addr = 0x68;
  _alpha = 0.98f;
  _sampleMs = 20;
  _lastMs = 0;
  _pitch = _roll = 0.0f;
  _count = 0;
  _recording = false;
  _hasResult = false;
  _buttonPin = 2;
  _resultLabel[0]=0;
}

bool GestureAirDrawPro::begin(uint8_t i2c_addr, uint8_t buttonPin){
  _addr = i2c_addr;
  _buttonPin = buttonPin;
  _wire->begin();
  // wake MPU6050
  _wire->beginTransmission(_addr);
  _wire->write(0x6B);
  _wire->write(0x00);
  _wire->endTransmission();
  pinMode(_buttonPin, INPUT_PULLUP);
  calibrate();
  // avoid large dt on first update
  _lastMs = millis();
  return true;
}

void GestureAirDrawPro::calibrate(){
  float ax,ay,az,gx,gy,gz;
  readRawAccelGyro(ax,ay,az,gx,gy,gz);
  _pitch = atan2f(ay, sqrtf(ax*ax + az*az)) * RAD_TO_DEG;
  _roll  = atan2f(-ax, az) * RAD_TO_DEG;
}

void GestureAirDrawPro::startRecording(){
  _count = 0;
  _recording = true;
  _hasResult = false;
  _resultLabel[0]=0;
}

void GestureAirDrawPro::stopRecording(){
  _recording = false;
  if(_count < 6){ _hasResult = false; return; }
  // make a copy and normalize
  Point temp[MAX_SAMPLES];
  for(int i=0;i<_count;i++) temp[i]=_buf[i];
  normalizeToUnit(temp,_count);
  // resample
  Point sample[TEMPLATE_POINTS];
  resample(temp,_count,sample,TEMPLATE_POINTS);
  // compare with templates (generate on the fly)
  float best = 1e9f; int bestId = -1;
  for(int id=0; id<NUM_TEMPLATES; id++){
    Point templ[TEMPLATE_POINTS];
    fillTemplate(templ, TEMPLATE_POINTS, id);
    // templ currently in -1..1, map to 0..1000 like normalized sample
    for(int i=0;i<TEMPLATE_POINTS;i++){
      templ[i].x = (templ[i].x + 1.0f) * 500.0f;
      templ[i].y = (templ[i].y + 1.0f) * 500.0f;
    }
    float d = dtwDistance(sample, TEMPLATE_POINTS, templ, TEMPLATE_POINTS);
    if(d < best){ best = d; bestId = id; }
  }
  if(bestId >= 0 && best < 60.0f){ // threshold tuned empirically
    if(bestId < 26){ _resultLabel[0] = 'A' + bestId; _resultLabel[1]=0; }
    else { int digit = bestId - 26; _resultLabel[0] = '0' + digit; _resultLabel[1]=0; }
    _hasResult = true;
  } else {
    _hasResult = false;
  }
}

bool GestureAirDrawPro::availableResult() const { return _hasResult; }
const char* GestureAirDrawPro::getResultLabel() const { return _resultLabel; }

void GestureAirDrawPro::printSVG(Print &out) const {
  out.print("<svg xmlns='http://www.w3.org/2000/svg' viewBox='0 0 1000 1000'>\n");
  out.print("<polyline fill='none' stroke='black' stroke-width='4' points='");
  for(int i=0;i<_count;i++){
    char buf[64];
    snprintf(buf,sizeof(buf), "%0.1f,%0.1f", _buf[i].x, _buf[i].y);
    out.print(buf);
    if(i+1 < _count) out.print(' ');
  }
  out.print("'/>\n");
  out.print("\n</svg>\n");
}

void GestureAirDrawPro::setSampleMs(unsigned long ms){ if(ms>=5) _sampleMs = ms; }
void GestureAirDrawPro::setAlpha(float a){ if(a>0.0f && a<1.0f) _alpha = a; }

void GestureAirDrawPro::normalizeToUnit(Point *arr, int n){
  float xmin=1e9f, xmax=-1e9f, ymin=1e9f, ymax=-1e9f;
  for(int i=0;i<n;i++){
    if(arr[i].x < xmin) xmin = arr[i].x; if(arr[i].x > xmax) xmax = arr[i].x;
    if(arr[i].y < ymin) ymin = arr[i].y; if(arr[i].y > ymax) ymax = arr[i].y;
  }
  float cx = (xmin + xmax) / 2.0f;
  float cy = (ymin + ymax) / 2.0f;
  float sx = (xmax - xmin) / 2.0f; if(sx < 1e-3f) sx = 1.0f;
  float sy = (ymax - ymin) / 2.0f; if(sy < 1e-3f) sy = 1.0f;
  float s = sx > sy ? sx : sy;
  for(int i=0;i<n;i++){
    float nx = (arr[i].x - cx) / s;
    float ny = (arr[i].y - cy) / s;
    arr[i].x = (nx + 1.0f) * 500.0f;
    arr[i].y = (ny + 1.0f) * 500.0f;
  }
}

void GestureAirDrawPro::resample(const Point *src, int nsrc, Point *dst, int ndst){
  if(nsrc <= 1){ for(int i=0;i<ndst;i++){ dst[i] = src[0]; } return; }
  for(int i=0;i<ndst;i++){
    float t = (float)i / (ndst - 1);
    float pos = t * (nsrc - 1);
    int i0 = floor(pos);
    int i1 = i0 + 1; if(i1 >= nsrc) i1 = nsrc - 1;
    float f = pos - i0;
    dst[i].x = src[i0].x * (1.0f - f) + src[i1].x * f;
    dst[i].y = src[i0].y * (1.0f - f) + src[i1].y * f;
  }
}

float GestureAirDrawPro::dtwDistance(const Point *a, int na, const Point *b, int nb){
  // Memory-efficient DTW: only two rows (previous and current) are stored.
  // Assumes TEMPLATE_POINTS is compile-time known and not huge. Uses ~2*(M+1)*4 bytes.
  const float INF = 1e9f;
  int N = na;
  int M = nb;
  // Static sized arrays for embedded environments:
  static float prevRow[TEMPLATE_POINTS + 1];
  static float currRow[TEMPLATE_POINTS + 1];

  // initialize
  for(int j = 0; j <= M; ++j) prevRow[j] = INF;
  prevRow[0] = 0.0f;

  for(int i = 1; i <= N; ++i){
    currRow[0] = INF;
    for(int j = 1; j <= M; ++j){
      float dx = a[i-1].x - b[j-1].x;
      float dy = a[i-1].y - b[j-1].y;
      float cost = sqrtf(dx*dx + dy*dy);
      float m = prevRow[j];
      if (currRow[j-1] < m) m = currRow[j-1];
      if (prevRow[j-1] < m) m = prevRow[j-1];
      currRow[j] = cost + m;
    }
    // swap rows (copy curr -> prev)
    for(int j = 0; j <= M; ++j) prevRow[j] = currRow[j];
  }
  // prevRow[M] now holds the total cost
  return prevRow[M] / (N + M);
}

void GestureAirDrawPro::fillTemplate(Point *out, int n, int id){
  for(int i=0;i<n;i++){ out[i].x = 0; out[i].y = 0; }
  if(id < 26){
    char ch = 'A' + id;
    for(int i=0;i<n;i++){
      float t = (float)i / (n - 1);
      switch(ch){
        case 'A': out[i].x = -1.0f + 2.0f*t; out[i].y = 1.0f - fabsf(out[i].x) * 1.8f; break;
        case 'B': out[i].x = -0.6f + 1.2f * sinf(t * 3.14159f * 2.0f); out[i].y = cosf(t * 3.14159f * 2.0f); break;
        case 'C': { float a = -1.2f + 2.4f * t; out[i].x = cosf(a); out[i].y = sinf(a); } break;
        case 'D': { float a = -1.0f + 2.0f * t; out[i].x = cosf(a*1.2f); out[i].y = sinf(a*1.2f); } break;
        case 'E': if(t < 0.5f){ out[i].x = -1.0f; out[i].y = -1.0f + 4.0f * t; } else { out[i].x = -1.0f + 2.0f * (t-0.5f); out[i].y = 1.0f; } break;
        default: out[i].x = cosf(t * 2.0f * 3.14159f + id); out[i].y = sinf(t * 2.0f * 3.14159f + id*0.5f); break;
      }
    }
  } else {
    int d = id - 26;
    for(int i=0;i<n;i++){
      float t = (float)i / (n - 1);
      switch(d){
        case 0: { float a = t * 2.0f * 3.14159f; out[i].x = cosf(a); out[i].y = sinf(a); } break;
        case 1: out[i].x = 0.0f; out[i].y = -1.0f + 2.0f * t; break;
        case 2: out[i].x = -1.0f + 2.0f * t; out[i].y = sinf(t * 3.14159f); break;
        case 3: out[i].x = sinf(t * 6.28318f); out[i].y = sinf(t * 6.28318f) * 0.8f; break;
        case 4: if(t<0.6f){ out[i].x = -0.6f; out[i].y = -1.0f + (t/0.6f)*2.0f; } else { out[i].x = -0.6f + (t-0.6f)/0.4f*1.2f; out[i].y = 0.2f; } break;
        case 5: out[i].x = -1.0f + 2.0f * t; out[i].y = sinf((t-0.25f) * 3.14159f * 1.5f); break;
        case 6: out[i].x = cosf(t * 6.28318f) * 0.6f - 0.2f; out[i].y = sinf(t * 6.28318f) * 0.8f; break;
        case 7: out[i].x = -1.0f + 2.0f * t; out[i].y = -1.0f + (t<0.5f?2.0f*t:1.0f); break;
        case 8: out[i].x = cosf(t * 6.28318f) * 0.5f; out[i].y = sinf(t * 6.28318f) * 0.8f; break;
        case 9: out[i].x = cosf(t * 6.28318f) * 0.5f + 0.3f; out[i].y = sinf(t * 6.28318f) * 0.6f; break;
        default: out[i].x = cosf(t*6.28318f); out[i].y = sinf(t*6.28318f); break;
      }
    }
  }
}

// low-level MPU6050 read
void GestureAirDrawPro::readRawAccelGyro(float &ax,float &ay,float &az,float &gx,float &gy,float &gz){
  _wire->beginTransmission(_addr);
  _wire->write(0x3B);
  _wire->endTransmission(false);
  uint8_t got = _wire->requestFrom((int)_addr, (int)14);
  if(got < 14){
    ax=ay=az=gx=gy=gz=0.0f;
    return;
  }
  int16_t rawAx = (_wire->read()<<8) | _wire->read();
  int16_t rawAy = (_wire->read()<<8) | _wire->read();
  int16_t rawAz = (_wire->read()<<8) | _wire->read();
  _wire->read(); _wire->read(); // temp / skip
  int16_t rawGx = (_wire->read()<<8) | _wire->read();
  int16_t rawGy = (_wire->read()<<8) | _wire->read();
  int16_t rawGz = (_wire->read()<<8) | _wire->read();
  ax = rawAx / 16384.0f; ay = rawAy / 16384.0f; az = rawAz / 16384.0f;
  gx = rawGx / 131.0f; gy = rawGy / 131.0f; gz = rawGz / 131.0f;
}

void GestureAirDrawPro::mapOrientationToXY(float &x,float &y){
  float dp = _pitch;
  float dr = _roll;
  x = 500.0f + dr * 6.5f;
  y = 500.0f - dp * 6.5f;
  x = constrain(x, 0.0f, 1000.0f);
  y = constrain(y, 0.0f, 1000.0f);
}

void GestureAirDrawPro::update(){
  unsigned long now = millis();
  if(now - _lastMs < _sampleMs) return;
  float dt = (now - _lastMs) / 1000.0f;
  _lastMs = now;
  static int prevBtn = HIGH;
  int btn = digitalRead(_buttonPin);
  if(btn == LOW && prevBtn == HIGH){ if(!_recording) startRecording(); else stopRecording(); }
  prevBtn = btn;
  float ax,ay,az,gx,gy,gz;
  readRawAccelGyro(ax,ay,az,gx,gy,gz);
  float accelPitch = atan2f(ay, sqrtf(ax*ax + az*az)) * RAD_TO_DEG;
  float accelRoll  = atan2f(-ax, az) * RAD_TO_DEG;
  _pitch += gx * dt; _roll  += gy * dt;
  _pitch = _alpha * _pitch + (1.0f - _alpha) * accelPitch;
  _roll  = _alpha * _roll  + (1.0f - _alpha) * accelRoll;
  float x,y; mapOrientationToXY(x,y);
  if(_recording && _count < MAX_SAMPLES){ _buf[_count].x = x; _buf[_count].y = y; _count++; }
}
