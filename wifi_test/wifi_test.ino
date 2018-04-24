#define AIN1 32
#define AIN2 23 
#define PWMA 4
#define BIN1 25
#define BIN2 26
#define PWMB 27
#define STBYAB 33

#define BRAKE 0
#define FORWARD 1
#define BACKWARDS 2
#define TURNRIGHT 3
#define TURNLEFT 4


#include <WiFi.h>

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

int state = 0;
unsigned long timer;

void setup() {

  Serial.begin(115200);

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

  timer = millis();
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
}
   


