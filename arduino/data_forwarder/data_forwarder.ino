/*
  data_forwarder.ino
  Streams the motion sensor over USB serial for the Edge Impulse data
  forwarder. Use this sketch to record your training data.

  Board:   Arduino Nano 33 BLE Sense Rev2 (BMI270 / BMM150 IMU)
           Original board (LSM9DS1)? Use #include <Arduino_LSM9DS1.h> instead.
  Output:  accX,accY,accZ,gyrX,gyrY,gyrZ   (one line per reading, numbers only)

  Notes
  - Numbers only. The forwarder rejects any text.
  - A fast-flashing light means the motion sensor failed to start. Check that
    your board version matches the IMU library included below.
  - The forwarder usually reports about 72 Hz with this sketch. Use the same
    sketch for every recording in a project.
  - Close the Serial Monitor before running edge-impulse-data-forwarder.
*/

#include <Arduino_BMI270_BMM150.h>

void setup() {
  Serial.begin(115200);

  pinMode(LED_BUILTIN, OUTPUT);

  if (!IMU.begin()) {
    // No serial text allowed, so flash the LED fast if the IMU fails
    while (1) {
      digitalWrite(LED_BUILTIN, HIGH);
      delay(100);
      digitalWrite(LED_BUILTIN, LOW);
      delay(100);
    }
  }
}

void loop() {
  float ax, ay, az, gx, gy, gz;

  // The Rev2 IMU samples accel and gyro at about 100 Hz,
  // so reading whenever both are ready gives a steady stream
  if (IMU.accelerationAvailable() && IMU.gyroscopeAvailable()) {
    IMU.readAcceleration(ax, ay, az);
    IMU.readGyroscope(gx, gy, gz);

    Serial.print(ax); Serial.print(',');
    Serial.print(ay); Serial.print(',');
    Serial.print(az); Serial.print(',');
    Serial.print(gx); Serial.print(',');
    Serial.print(gy); Serial.print(',');
    Serial.println(gz);
  }
}
