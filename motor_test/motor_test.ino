#define AIN1 32
#define AIN2 23 
#define PWMA 4
#define BIN1 25
#define BIN2 26
#define PWMB 27
#define STBYAB 33
#define SDA 21
#define SCL 22

#define lTrig 16
#define lEcho 17
#define rTrig 14
#define rEcho 12
#define fTrig 5
#define fEcho 15

#define led 18

#define BRAKE 0
#define FORWARD 1
#define BACKWARDS 2
#define TURNRIGHT 3
#define TURNLEFT 4

#define WAIT 0
#define PARSE 1
#define TURN 2
#define STARTUP 3
#define DRIVE 4
#define CHECKDOOR 5
#define DOORWAY 6
#define ADJUST 7

#include <WiFi.h>
#include <mpu9255_esp32.h>

MPU9255 imu; //imu object called, appropriately, imu

float lDist, rDist, fDist;
float dist;

const int d_c = 10;
const int AVG_HISTORY_LEN = 10;
const int d_threshhold = 50;

float p, i=0, d;
float d_history[d_c] = {0};
float d_avg_history[AVG_HISTORY_LEN] = {0};

float p_w = 4;
float i_w = 0.0001;
float d_w = 1000;
float max_delta = 20;
float max_curve = 30;

float last_error = 0;
int count = 0;
int check_count = 0;

int state = 0;
int d_state = WAIT;
unsigned long door_timer;
unsigned long timer;
unsigned long timer2;
unsigned long person_timer;

int door_count = 0;
int max_doors = 0;
bool side = true;
bool turn_dir;
String cur_instr;
int instr_n;  
bool go = false;

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
  const float offset = 15;

  public:
  Robot(Motor l, Motor r) {

    left = l;
    right = r;

  }

  void fwd(int spd) {
    left.fwd(spd - offset);
    right.fwd(spd);
  }

  void back(int spd) {
    left.back(spd - offset);
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

  pinMode(lTrig, OUTPUT);
  pinMode(lEcho, INPUT);
  pinMode(rTrig, OUTPUT);
  pinMode(rEcho, INPUT);
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

  setup_imu();

  timer = millis();
  timer2 = millis();
  door_timer = millis();
  person_timer = millis();
}

void setup_imu() {
  //setup imu
  if (imu.readByte(MPU9255_ADDRESS, WHO_AM_I_MPU9255) == 0x73){
    imu.initMPU9255();
  }else{
    while(1) Serial.println("NOT FOUND"); // Loop forever if communication doesn't happen
  }
  imu.getAres(); //call this so the IMU internally knows its range/resolution
  delay(50);
  imu.calibrateMPU9255(imu.gyroBias, imu.accelBias);
  delay(50);
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


  float error;

  if (side) {
    error = getDist(rTrig,rEcho) - 40;
  }
  else {
    error = 10 - getDist(lTrig,lEcho);
  }
  
  p = p_w * error;
  d = d_w * (error - last_error) / (millis() - timer2);
  i += i_w * error * (millis() - timer2);
  float d_avg;
  float c;

  String instr[] = {"r", "2l", "r", "1l", "l", "1r", "r"};
  go = true;

  switch(d_state) {
    case WAIT:
      if (go) {
        d_state = PARSE;
        instr_n = 0;
      }
      break;
    case PARSE:
      if (instr_n == *(&instr + 1) - instr) {
        go = false;
        d_state = WAIT;
      }
      else {
        cur_instr = instr[instr_n];
        instr_n += 1;
        if ((int) cur_instr.charAt(0) >= 49 && (int) cur_instr.charAt(0) <= 57) {
          max_doors = (int) (cur_instr.charAt(0)) - 48;
          side = (cur_instr.charAt(1) == 'r');
          d_state = STARTUP;
          count = 0;
        }
        else {
          turn_dir = (cur_instr.charAt(0) == 'r');
          d_state = TURN;
        }
      }
      break;
    case TURN:
      if (turn_dir) {
        turn(90, 200);
      }
      else {
        turn(-90, 200);
      }
      d_state = PARSE;
      break;
    case STARTUP:
      d_avg = updateDHistory();
      
      if (count < 20) {
        count++;
      }
      else {
        d_state = DRIVE;
      }
      break;
    case DRIVE:
      d_avg = updateDHistory();
      
      c = p + d_avg + i;
      if(c > max_curve)
        c = max_curve;
      else if(c < -1 * 50)
        c = -1 * 50;
        
      r.curve(150, c);

      if ((d_avg_history[0]-d_avg_history[1] < -50 && side) || (d_avg_history[0]-d_avg_history[1] > 50 && !side)) {
        Serial.print("PERSON");
        person_timer = millis();
      }
      else if (((d_avg_history[0]-d_avg_history[1] > d_threshhold && side) || (d_avg_history[0]-d_avg_history[1] < -1* d_threshhold && !side)) && millis() - person_timer > 2000) {
        r.brake();
        d_state = CHECKDOOR;
        check_count = 0;
      }
      break;
    case CHECKDOOR:
      d_avg = updateDHistory();

      if(millis() - door_timer > 5000) {
        int door_measurements = 0;
        for(int i = 0; i < AVG_HISTORY_LEN - 1; i++) {
          if (side) {
            if(d_avg_history[i] - d_avg_history[AVG_HISTORY_LEN - 1] > d_threshhold) {
              door_measurements += 1;
            }
          }
          else {
            if(d_avg_history[i] - d_avg_history[AVG_HISTORY_LEN - 1] < -1 * d_threshhold) {
              door_measurements += 1;
            }
          }
        }
        
        if(door_measurements >= 3) {
          d_state = DOORWAY;
          door_timer = millis();
        }
      }

      check_count++;
      if(check_count > 10) {
        d_state = DRIVE;
      }
      break;
    case DOORWAY:
      if(millis() - door_timer < 2000) {
        digitalWrite(led, HIGH);
        door_count += 1;
        r.fwd(150);
      }
      else {
        digitalWrite(led, LOW);
        if (door_count = max_doors) {
          d_state = ADJUST;
        }
        else {
          d_state = DRIVE;          
        }
      }
      break;
    case ADJUST:
      r.fwd(150);
      delay(500);
      r.brake();
      d_state = PARSE;
      break;
  }


  Serial.print(d_state);
  Serial.print(" ");
  Serial.print(d_avg);
  Serial.print(" ");


  timer2 = millis();
  if(d_state != CHECKDOOR) {
    last_error = error;
  }
  else {
    Serial.print(" d: " + String(d) + " ");
  }
  Serial.print(error);
  Serial.print(" ");
  Serial.println(last_error);

  while (millis() - timer < 50);
  timer = millis();
  
}

float updateDHistory() {
  // shift entries in d_history to filter d 
  float d_avg;
    
  if((d < 200 && side) || (d > -200 && !side)) {
    for (int q = d_c - 1; q > 0; q--) {
      d_history[q] = d_history[q-1];
    }
    d_history[0] = d;
  
    // compute d_avg to check for doorway
    float sum = 0;
  
    for (int q = 0; q < d_c; q++) {
      sum += d_history[q];
    }
  
    d_avg = sum / d_c;
  
    // shift entries in d_avg_history
    for (int q = AVG_HISTORY_LEN - 1; q > 0; q--) {
      d_avg_history[q] = d_avg_history[q-1];
    }
    d_avg_history[0] = d_avg;
  }
  else {
    // shift entries in d_avg_history
    for (int q = AVG_HISTORY_LEN - 1; q > 0; q--) {
      d_avg_history[q] = d_avg_history[q-1];
    }
    if (side) {
      d_avg = 200;
    }
    else {
      d_avg = -200;
    }
    d_avg_history[0] = d_avg;
  }

  return d_avg;
}

void turn(int dgrees, int spd) {

  const float g_const = -0.00077;
  const float epsilon = 10;

  float angle = 0;
  Serial.println("angle: " + String(angle));
  while(abs(angle-dgrees)>epsilon) {
    if(dgrees > 0)
      r.turnRight(spd);
    else {
      r.turnLeft(spd);
    }
    imu.readGyroData(imu.gyroCount);
    if(imu.gyroCount[2]*g_const > 0.1 || imu.gyroCount[2]*g_const < -0.1)
      angle += imu.gyroCount[2]*g_const;
    Serial.println("angle: " + String(angle));
    delay(100);
  }
  r.brake();
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
   


