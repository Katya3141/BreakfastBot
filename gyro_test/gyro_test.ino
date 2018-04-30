#include <mpu9255_esp32.h>

float angle=0;
MPU9255 imu; //imu object called, appropriately, imu
const float g_const = -0.00077;

void setup_imu(){
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

void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
  setup_imu();
}

void loop() {
  imu.readGyroData(imu.gyroCount);
  if(imu.gyroCount[2]*g_const > 0.1 || imu.gyroCount[2]*g_const < -0.1)
    angle += imu.gyroCount[2]*g_const;
  

  Serial.print(angle);
  Serial.println(" ");

  delay(100);
}

/*void turn(int dgrees, int spd) {
    int angle = 0;
    while(abs(angle-dgrees)>epsilon) {
      if(dgrees > 0)
        turnRight(spd);
      else
        turnLeft(spd);
      imu.readGyroData(imu.gyroCount);
      if(imu.gyroCount[2]*g_const > 0.1 || imu.gyroCount[2]*g_const < -0.1)
        angle += imu.gyroCount[2]*g_const;
    }
    brake();
  }*/
