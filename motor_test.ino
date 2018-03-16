#define AIN1 2
#define AIN2 4
#define PWMA 5
#define BIN1 7
#define BIN2 8
#define PWMB 6
#define STBYAB 19

class Motor {
  public:
  int in1, in2, pwm, channel, stdby;
  int freq = 5000;
  int resolution = 8;
  
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

  void drive(int spd) {
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

Motor A(AIN1, AIN2, PWMA, 0, STBYAB);

void setup() {

}

void loop() {
   A.drive(150);
   delay(2000);
   A.back(200);
   delay(2000);
   A.brake();
   delay(5000);
}


