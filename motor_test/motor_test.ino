#define AIN1 32
#define AIN2 23 
#define PWMA 4
#define BIN1 25
#define BIN2 26
#define PWMB 27
#define STBYAB 33
#define SDA 21
#define SCL 22

#define r2Trig 16
#define r2Echo 17
#define r1Trig 14
#define r1Echo 12
#define fTrig 5
#define fEcho 15

#define led 18

#define BRAKE 0
#define FORWARD 1
#define BACKWARDS 2
#define TURNRIGHT 3
#define TURNLEFT 4


#include <WiFi.h>

float r2Dist, r1Dist, fDist;

const int d_c = 10;

float p, i=0, d;
float d_history[d_c] = {0};
float d_avg_history[5] = {0};

float p_w = 4;
float i_w = 0.0001;
float d_w = 1000;
float max_delta = 20;
float max_curve = 30;

float last_error = 0;
int count = 0;

int state = 0;
unsigned long door_timer;
unsigned long timer;
unsigned long timer2;

float getDist(int trig, int echo) {
  float duration, distance;
  
  digitalWrite(trig, LOW);
  delayMicroseconds(2);
  digitalWrite(trig, HIGH);
  delayMicroseconds(10);
  digitalWrite(trig, LOW);

  duration = pulseIn(echo, HIGH);
  distance = (duration/2)/29.1;

  return distance;
}

class Motor {

  int in1, in2, pwm, channel, stdby;
  int freq = 5000;
  int resolution = 8;

  public:

  Motor() {
    
  }
  
  Motor(int in1_pin, int in2_pin, int pwm_pin, int channel_pin, int stdby_pin) {
    in1 = in1_pin;
    in2 = in2_pin;
    pwm = pwm_pin;
    channel = channel_pin;
    stdby = stdby_pin;

    pinMode(in1, OUTPUT);
    pinMode(in2, OUTPUT);
    pinMode(pwm, OUTPUT);
    pinMode(stdby, OUTPUT);
    
    ledcSetup(channel, freq, resolution);
    ledcAttachPin(pwm, channel);
  }

  void fwd(int spd) {
   digitalWrite(in1, LOW);
   digitalWrite(in2, HIGH);
   ledcWrite(channel, spd);
   digitalWrite(stdby, HIGH);
  }

  void back(int spd) {
   digitalWrite(in1, HIGH);
   digitalWrite(in2, LOW);
   ledcWrite(channel, spd);
   digitalWrite(stdby, HIGH);
  }
  
  void brake() {
   digitalWrite(in1, LOW);
   digitalWrite(in2, LOW);
   digitalWrite(pwm, HIGH);
   digitalWrite(stdby, HIGH);
  }
};

class Robot {
  Motor left, right;

  public:
  Robot(Motor l, Motor r) {
    left = l;
    right = r;
  }

  void fwd(int spd) {
    left.fwd(spd);
    right.fwd(spd);
  }

  void back(int spd) {
    left.back(spd);
    right.back(spd);
  }

  void turnRight(int spd) {
    left.fwd(spd);
    right.back(spd);
  }

  void turnLeft(int spd) {
    left.back(spd);
    right.fwd(spd);
  }

  void brake() {
    left.brake();
    right.brake();
  }

  void curve(int spd, int curve) {
    if (curve < 0) {
      left.fwd(spd + curve);
      right.fwd(spd);
    }
    else {
      left.fwd(spd);
      right.fwd(spd - curve);
    }
  }

  void curveBack(int spd, int curve) {
    if (curve > 0) {
      left.back(spd);
      right.back(spd - curve);
    }
    else {
      left.back(spd + curve);
      right.back(spd);
    }
  }
};

Motor A(AIN1, AIN2, PWMA, 0, STBYAB);
Motor B(BIN1, BIN2, PWMB, 1, STBYAB);
Robot r(A, B);

void setup() {

  Serial.begin(115200);

  pinMode(r2Trig, OUTPUT);
  pinMode(r2Echo, INPUT);
  pinMode(r1Trig, OUTPUT);
  pinMode(r1Echo, INPUT);
  pinMode(fTrig, OUTPUT);
  pinMode(fEcho, INPUT);
  pinMode(led, OUTPUT);

  delay(100); //wait a bit (100 ms)
  WiFi.begin("MIT"); //attempt to connect to wifi
  int count = 0; //count used for Wifi check times
  while (WiFi.status() != WL_CONNECTED && count<6) {
    delay(500);
    Serial.print(".");
    count++;
  }
  delay(2000);
  if (WiFi.isConnected()) { //if we connected then print our IP, Mac, and SSID we're on
    Serial.println(WiFi.localIP().toString() + " (" + WiFi.macAddress() + ") (" + WiFi.SSID() + ")");
    delay(500);
  } else { //if we failed to connect just ry again.
    Serial.println(WiFi.status());
    ESP.restart(); // restart the ESP
  }

  String weights = getWeights();
  int space1 = weights.indexOf(" ");
  int space2 = weights.indexOf(" ", space1 + 1);
  int space3 = weights.indexOf(" ", space2 + 1);
  int space4 = weights.indexOf(" ", space3 + 1);

  p_w = weights.substring(0, space1).toFloat();
  i_w = weights.substring(space1 + 1, space2).toFloat();
  d_w = weights.substring(space2 + 1, space3).toFloat();
  max_delta = weights.substring(space3 + 1, space4).toFloat();
  max_curve = weights.substring(space4 + 1).toFloat();

  timer = millis();
  timer2 = millis();
  door_timer = millis();
}

String getInstruction(){
  WiFiClient client; //instantiate a client object
  if (client.connect("iesc-s1.mit.edu", 80)) { //try to connect to numbersapi.com host
    // This will send the request to the server
    // If connected, fire off HTTP GET:
    client.println("GET /608dev/sandbox/pwang21/breakfastbot.py HTTP/1.1"); 
    client.println("Host: iesc-s1.mit.edu");
    client.print("\r\n");
    unsigned long count = millis();
    while (client.connected()) { //while we remain connected read out data coming back
      String line = client.readStringUntil('\n');
      //Serial.println(line);
      if (line == "\r") { //found a blank line!
        //headers have been received! (indicated by blank line)
        break;
      }
      if (millis()-count>6000) break;
    }
    count = millis();
    String op; //create empty String object
    while (client.available()) { //read out remaining text (body of response)
      op+=(char)client.read();
    }
    return op;
    client.stop();
  }else{
    Serial.println("connection failed");
    Serial.println("wait 0.5 sec...");
    client.stop();
  }
}

void loop() {

  // wifi control

  /*
  
  int temp = state;
  state = getInstruction().toInt();
  if (state != temp) {
    switch (state) {
      case BRAKE:
        r.brake();
        break;
      case FORWARD:
        r.fwd(250);
        break;
      case BACKWARDS:
        r.back(250);
        break;
      case TURNRIGHT:
        r.turnRight(250);
        break;
      case TURNLEFT:
        r.turnLeft(250);
        break;
    }
    while (millis() - timer < 30);
    timer = millis();
  }
  Serial.println(String(state));

  */

  
  r1Dist = getDist(r1Trig,r1Echo);

  float error = r1Dist - 40;
    
  if (r1Dist < 200) {
    
    p = p_w * error;
    d = d_w * (error - last_error) / (millis() - timer2);
    i += i_w * error * (millis() - timer2);


    // shift entries in d_history    
    int d_sum = 0;

    for (int q = d_c - 1; q > 0; q--) {
      d_history[q] = d_history[q-1];
    }
    d_history[0] = d;


    // compute d_avg
    float sum = 0;

    for (int q = 0; q < d_c; q++) {
      sum += d_history[q];
    }

    float d_avg = sum / d_c;

    // shift entries in d_avg_history
    for (int q = 4; q > 0; q--) {
      d_avg_history[q] = d_avg_history[q-1];
    }
    d_avg_history[0] = d_avg;

    // check for doorway
    if (millis() - door_timer > 5000) {
      for (int q = 4; q > 0; q--) {
        if (abs(d_avg_history[q] - d_avg_history[0]) > 50) {
          door_timer = millis();
          digitalWrite(led, HIGH);
          delay(100);
          digitalWrite(led, LOW);
          break;
        }
      }
    }
    

    timer2 = millis();
    last_error = error;
    float c = p + d_avg + i;
    if(c > max_curve)
      c = max_curve;
    else if(c < -1 * max_curve)
      c = -1 * max_curve;


    if (count < 20) {
      count++;
    }
    else {
      r.curve(150, c);
      
      Serial.print(d_avg);
      Serial.println(" ");

      /*
      Serial.println(c); 
      */
    }
  }
  else {
    r.brake();
  }

  while (millis() - timer < 50);
  timer = millis();
  
}

String getWeights(){
  WiFiClient client; //instantiate a client object
  if (client.connect("iesc-s1.mit.edu", 80)) { //try to connect to numbersapi.com host
    // This will send the request to the server
    // If connected, fire off HTTP GET:
    client.println("GET /608dev/sandbox/pwang21/breakfastbotweights.py HTTP/1.1"); 
    client.println("Host: iesc-s1.mit.edu");
    client.print("\r\n");
    unsigned long count = millis();
    while (client.connected()) { //while we remain connected read out data coming back
      String line = client.readStringUntil('\n');
      //Serial.println(line);
      if (line == "\r") { //found a blank line!
        //headers have been received! (indicated by blank line)
        break;
      }
      if (millis()-count>6000) break;
    }
    count = millis();
    String op; //create empty String object
    while (client.available()) { //read out remaining text (body of response)
      op+=(char)client.read();
    }
    return op;
    client.stop();
  }else{
    Serial.println("connection failed");
    Serial.println("wait 0.5 sec...");
    client.stop();
  }
}
   


