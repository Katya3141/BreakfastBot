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
#define led2 13

#define LISTEN 1
#define DISPENSE 2
#define PARSE 3
#define WAIT 4
#define TURN 5
#define FINDWALL 6
#define STARTUP 7
#define DRIVE 8
#define CHECKIN 9
#define CHECKOUT 10
#define ADJUST 11
#define DOCK 12

#include <WiFi.h>
#include <mpu9255_esp32.h>
#include <math.h>

MPU9255 imu; //imu object called, appropriately, imu

float p_w, i_w, d_w, max_curve;
const int loop_time = 50;

float last_error[10] = {0};
float offset[10];
float normalized[10];
int history = 10;

int state = LISTEN;
int in_out_state = 0;

int side;
int max_doors;
bool turn_dir = true;

int startup_count = 0;
int door_count = 0;

bool check_last_door;
int check_last_door_count = 0;

String instr;
String current_instr;
int instr_n = 0;

int choice;

unsigned long timer, last_measure_time, startup_timer, last_end_door_time, last_start_door_time, last_start_person_time, docking_timer;

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
      left.fwd(spd + curve - offset);
      right.fwd(spd);
    }
    else {
      left.fwd(spd - offset);
      right.fwd(spd - curve);
    }
  }

  void curveBack(int spd, int curve) {
    if (curve > 0) {
      left.back(spd - offset);
      right.back(spd - curve);
    }
    else {
      left.back(spd + curve - offset);
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
  pinMode(led2, OUTPUT);

  delay(100); //wait a bit (100 ms)
  WiFi.begin("MIT"); //attempt to connect to wifi
  //WiFi.begin("6s08","iesc6s08");
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
  } else { //if we failed to connect just try again.
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
  instr = weights.substring(space3 + 1, space4);
  max_curve = weights.substring(space4 + 1).toFloat();

  setup_imu();

  timer = millis();
  last_measure_time = millis();
}

void loop() {

  float error, c, p, d, i;
  
  if (side == 1) {
    error = getDist(rTrig,rEcho) - 40;
  }
  else {
    error = 30 - getDist(lTrig,lEcho);
  }

  p = p_w * error;
  d = d_w * (error - last_error[0]) / (millis() - last_measure_time);
  i += i_w * error * (millis() - last_measure_time);
  c = p + d + i;

  last_measure_time = millis();

  if(c > max_curve)
    c = max_curve;
  else if(c < -1 * max_curve)
    c = -1 * max_curve;

  switch (state) {
    case LISTEN:
      instr = getInstructions();
      if (instr!="-1") {
        instr_n = 0;
        state = DISPENSE;
      }
      break;
    case DISPENSE:
      choice = getCerealInstructions().toInt();
      postDispState(choice);
      state = PARSE;
      break;
    case PARSE:
      if (instr_n == instr.length()) {
        state = WAIT;
      }
      else {
        current_instr = instr.substring(instr_n, instr_n + 2);
        if ((int) current_instr.charAt(0) >= 49 && (int) current_instr.charAt(0) <= 57) {
          max_doors = (int) (current_instr.charAt(0)) - 48;
          if (current_instr.charAt(1) == 'r') {
            side = 1;
          }
          else {
            side = -1;
          }
          state = STARTUP;
        }
        else if (current_instr.charAt(0) == 'd') {
          if (current_instr.charAt(1) == 'r') {
            side = 1;
          }
          else {
            side = -1;
          }
          state = DOCK;
        }
        else {
          turn_dir = (current_instr.charAt(1) == 'r');
          state = TURN;
        }
        instr_n += 2;
      }
      break;
    case WAIT:
      //wait
      reverseInstructions();
      state = PARSE;
      break;
    case TURN:
      if (turn_dir) {
        turn(80, 100);
      }
      else {
        turn(-80, 100);
      }
      state = FINDWALL;
      break;
    case FINDWALL:
      r.fwd(150);
      if (abs(error) < 150 && abs(last_error[0]) < 150 && abs(last_error[1]) < 150) {
        r.brake();
        state = PARSE;
      }
      break;
    case STARTUP:
      startup_count++;
      if (startup_count >= history + 5) {
        startup_timer = millis();
        state = DRIVE;
      }
      break;
    case DRIVE:
      if (millis() - last_start_door_time > 2800 && in_out_state == 1) {
        in_out_state = 0;
      }
      if (millis() - last_start_person_time > 2800 && in_out_state == -1) {
        in_out_state = 0;
      }
      
      if (offset_normalize() < 2.1 || millis() - startup_timer < 3000) {
        if (abs(error) > 200) {
          r.fwd(150);
        }
        else {
          r.curve(150, c);
        }
        check_last_door = false;
        check_last_door_count = 0;
        break;
      }

      
      for (int i = 0; i < history; i++) {
        Serial.print(normalized[i]);
        Serial.print(" ");
      }
      Serial.println("");

      for (int i = 0; i < history; i++) {
        Serial.print(last_error[i]);
        Serial.print(" ");
      }
      Serial.println("");

      if (check_last_door) {
        check_last_door_count++;
        if (check_last_door_count > 5) {
          state = ADJUST;
          in_out_state = 1;
          check_last_door = false;
          check_last_door_count = 0;
          break;
        }
      }
      
      if (startDoor() && millis() - last_start_door_time > 750 && millis() - last_end_door_time > 3000) {

        if (in_out_state < 1) {
          in_out_state++;
          last_start_door_time = millis();
        }
        if (door_count + 1 == max_doors && in_out_state == 1) {
          last_start_door_time = millis();
          check_last_door = true;
        }

        r.curve(150, side * -5);
      }
      else if (endDoor() && millis() - last_end_door_time > 3000) {
        if (in_out_state > -1) {
          in_out_state--;
        }
        if (in_out_state == -1) {
          last_start_person_time = millis();
        }
        else if (in_out_state == 0 && millis() - last_start_door_time > 300) {
          door_count++;
          last_end_door_time = millis();
        }
        
        check_last_door = false;
        check_last_door_count = 0;

        r.curve(150, side * 5);
      }
      else if (in_out_state == 0) {
        if (abs(error) > 200) {
          r.fwd(150);
        }
        else {
          r.curve(150, c);
        }
      }
      break;
    case ADJUST:
      if (instr_n + 4 > instr.length()) {
        //r.back(150);
        //delay(300);
      }
      else if (instr.charAt(instr_n + 1) == instr.charAt(instr_n + 3)) {
        //r.back(150);
        //delay(300);
      }
      else {
        r.fwd(150);
        delay(300);
      }
      r.brake();
      state = PARSE;
      in_out_state = 0;
      door_count = 0;
      break;
    case DOCK:
      turn(180, 110);
      docking_timer = millis();
      while(millis() - docking_timer < 1500) {
        r.curveBack(150,side*-10);
      }
      r.curveBack(150,side*25);
      delay(2500);
      r.brake();
      state = LISTEN;
      break;
  }
  
  if (in_out_state == -1) {
    digitalWrite(led2, HIGH);
    digitalWrite(led, LOW);
  }
  else if (in_out_state == 1) {
    digitalWrite(led, HIGH);
    digitalWrite(led2, LOW);
  }
  else {
    digitalWrite(led, LOW);
    digitalWrite(led2, LOW);
  }

  Serial.print(door_count);
  Serial.print(" ");
  Serial.print(in_out_state);
  Serial.print(" ");
  Serial.print(state);
  Serial.print(" ");
  Serial.print(check_last_door_count);
  Serial.println("");

  for (int i = history-1; i > 0; i--) {
    last_error[i] = last_error[i-1];
  }

  if (error < -200) {
    error = -200;
  }
  else if (error > 200) {
    error = 200;
  }
  last_error[0] = error;

  
  /*
  Serial.print(" ");
  Serial.print(side * (last_error[0] + last_error[1] + last_error[2] - last_error[3] - last_error[4] - last_error[5]));
  Serial.print(" ");
  Serial.print(error);
  Serial.print(" ");
  Serial.println(in_out_state);
  */
  
  while (millis() - timer < loop_time);
  timer = millis();
}

float offset_normalize() {
  float sum = 0;
  for (int i = 0; i < history; i++) {
    sum += last_error[i];
  }
  float avg = sum / history;
  
  for (int i = 0; i < history; i++) {
    offset[i] = last_error[i] - avg;
  }

  float sum2 = 0;
  for (int i = 0; i < history; i++) {
    sum2 += offset[i] * offset[i];
  }
  float norm = sqrt(sum2);
  
  for (int i = 0; i < history; i++) {
    normalized[i] = offset[i] / norm;
  }

  Serial.println(norm);

  return norm;
}

bool startDoor() {
  float sum = 0;
  for (int i = 0; i < history / 2; i++) {
    sum += normalized[i] * 1 / sqrt(history) * side;
  }
  for (int i = history / 2; i < history; i++) {
    sum += normalized[i] * -1 / sqrt(history) * side;
  }

  float sum2 = 0;
  for (int i = 0; i < history; i++) {
    float w = (history - 1) / 2.0 - i;
    sum2 += normalized[i] * w / 9.083 * side;
  }

  Serial.print(sum);
  Serial.print(" ");
  Serial.print(sum2);
  Serial.println("");
  Serial.println("-----------------------------------");

  return (sum > 0.5 && sum2 < sum + 0.05 && sum2 < 0.95);
}
bool endDoor() {
  float sum = 0;
  for (int i = 0; i < history / 2; i++) {
    sum += normalized[i] * -1 / sqrt(history) * side;
  }
  for (int i = history / 2; i < history; i++) {
    sum += normalized[i] * 1 / sqrt(history) * side;
  }

  float sum2 = 0;
  for (int i = 0; i < history; i++) {
    float w = (1 - history) / 2.0 + i;
    sum2 += normalized[i] * w / 9.083 * side;
  }

  return (sum > 0.65 && sum2 < sum + 0.05 && sum2 < 0.95);
  
  //return (side * (last_error[0] + last_error[1] + last_error[2] - last_error[3] - last_error[4] - last_error[5]) < -1 * dif_threshhold);
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

void turn(int dgrees, int spd) {

  const float g_const = -0.00077;
  const float epsilon = 5;

  float angle = 0;
  while(abs(angle-dgrees)>epsilon) {
    if(dgrees > 0)
      r.turnRight(spd);
    else {
      r.turnLeft(spd);
    }
    imu.readGyroData(imu.gyroCount);
    if(imu.gyroCount[2]*g_const > 0.1 || imu.gyroCount[2]*g_const < -0.1)
      angle += imu.gyroCount[2]*g_const;
    delay(100);
  }
  r.brake();
}

void reverseInstructions() {
  String temp = "";
  for (int x = instr.length()-2; x >=0; x-+2) {
//    current_instr = instr.substring(instr_n, instr_n + 2);
//      if ((int) current_instr.charAt(0) >= 49 && (int) current_instr.charAt(0) <= 57) {
//        max_doors = (int) (current_instr.charAt(0)) - 48;
//        if (current_instr.charAt(1) == 'r') {
//          side = 1;
//        }
//        else {
//          side = -1;
//        }
//        state = STARTUP;
//      }
//      else if (current_instr.charAt(0) == 'd') {
//        if (current_instr.charAt(1) == 'r') {
//          side = 1;
//        }
//        else {
//          side = -1;
//        }
//        state = DOCK;
//      }
//      else {
//        turn_dir = (current_instr.charAt(1) == 'r');
//        state = TURN;
//      }
//      instr_n += 2;
//    }
  }
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

String getInstructions(){
  WiFiClient client; //instantiate a client object
  if (client.connect("iesc-s1.mit.edu", 80)) { //try to connect to numbersapi.com host
    // This will send the request to the server
    // If connected, fire off HTTP GET:
    client.println("GET /608dev/sandbox/pwang21/breakfastbot.py?query=state HTTP/1.1"); 
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

String getCerealInstructions(){
  WiFiClient client; //instantiate a client object
  if (client.connect("iesc-s1.mit.edu", 80)) { //try to connect to numbersapi.com host
    // This will send the request to the server
    // If connected, fire off HTTP GET:
    client.println("GET /608dev/sandbox/pwang21/breakfastbot.py?query=cereal HTTP/1.1"); 
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

void postDispState(int dispState){
  WiFiClient client; //instantiate a client object
  if (client.connect("iesc-s1.mit.edu", 80)) { //try to connect
    // This will send the request to the server
    // If connected, fire off HTTP GET:
    String thing = "{\"state\":\""+String(dispState)+"\"}";
    client.println("POST /608dev/sandbox/kbulovic/dispenser.py HTTP/1.1");
    client.println("Host: iesc-s1.mit.edu");
    client.println("Content-Type: application/json");
    client.println("Content-Length: " + String(thing.length()));
    client.print("\r\n");
    client.print(thing);
    unsigned long count = millis();
    while (client.connected()) { //while we remain connected read out data coming back
      String line = client.readStringUntil('\n');
      Serial.println(line);
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
    Serial.println(op);
    client.stop();
    Serial.println();
    Serial.println("-----------");
  }else{
    Serial.println("connection failed");
    Serial.println("wait 0.5 sec...");
    client.stop();
    delay(300);
  }
}
