#include <mpu9255_esp32.h>

float r=0;
MPU9255 imu; //imu object called, appropriately, imu

void setup_imu(){
  if (imu.readByte(MPU9255_ADDRESS, WHO_AM_I_MPU9255) == 0x73){
    imu.initMPU9255();
  }else{
    while(1) Serial.println("NOT FOUND"); // Loop forever if communication doesn't happen
  }
  imu.getAres(); //call this so the IMU internally knows its range/resolution
  //imu.calibrateMPU9255(imu.gyroBias, imu.accelBias);
}

void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
  setup_imu();
}

void loop() {
  imu.readGyroData(imu.gyroCount);
  r += imu.gyroCount[2];
  

  Serial.print(r);
  Serial.println(" ");

  delay(100);
}
