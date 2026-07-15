#include <Wire.h>
#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>

// Change  when using other pins for the MPU-6050!
#define SDA_PIN 6
#define SCL_PIN 7

Adafruit_MPU6050 mpu;

const int samples = 2000;

// In case not on Earth f.e. moon or Mars, change G to account for that places specific gravity constant
// When traveling thru the galaxy, your lightsaber must always work, right!
const float G = 9.80665;

//=====================================================
// Select ONE orientation
//=====================================================

//#define FACE_XY_DOWN   // XY face on table -> Z points UP
//#define FACE_XY_UP     // XY face up -> Z points DOWN

//#define FACE_XZ_DOWN   // XZ face on table -> Y points UP
//#define FACE_XZ_UP     // XZ face up -> Y points DOWN

//#define FACE_YZ_DOWN   // YZ face on table -> X points UP
#define FACE_YZ_UP       // YZ face up -> X points DOWN
//=====================================================

void setup() {

  Serial.begin(115200);
  while (!Serial);

  Wire.begin(SDA_PIN, SCL_PIN);

  if (!mpu.begin()) {
    Serial.println("Could not find MPU6050!");
    while (1);
  }

  Serial.println();
  Serial.println("Keep the MPU6050 perfectly still.");
  Serial.println("Calibration starts in 5 seconds...");
  delay(5000);

  double axSum = 0;
  double aySum = 0;
  double azSum = 0;
  double gxSum = 0;
  double gySum = 0;
  double gzSum = 0;

  sensors_event_t a, g, t;

  for (int i = 0; i < samples; i++) {

    mpu.getEvent(&a, &g, &t);

    axSum += a.acceleration.x;
    aySum += a.acceleration.y;
    azSum += a.acceleration.z;

    gxSum += g.gyro.x;
    gySum += g.gyro.y;
    gzSum += g.gyro.z;

    delay(2);
  }

  float ax = axSum / samples;
  float ay = aySum / samples;
  float az = azSum / samples;

  float gx = gxSum / samples;
  float gy = gySum / samples;
  float gz = gzSum / samples;

  float axOffset = ax;
  float ayOffset = ay;
  float azOffset = az;

#ifdef FACE_XY_DOWN
  azOffset -= G;
#endif

#ifdef FACE_XY_UP
  azOffset += G;
#endif

#ifdef FACE_XZ_DOWN
  ayOffset -= G;
#endif

#ifdef FACE_XZ_UP
  ayOffset += G;
#endif

#ifdef FACE_YZ_DOWN
  axOffset -= G;
#endif

#ifdef FACE_YZ_UP
  axOffset += G;
#endif

  Serial.println();
  Serial.println("========== Calibration ==========");
  Serial.print("Accel X Offset: "); Serial.println(axOffset, 6);
  Serial.print("Accel Y Offset: "); Serial.println(ayOffset, 6);
  Serial.print("Accel Z Offset: "); Serial.println(azOffset, 6);

  Serial.println();

  Serial.print("Gyro X Offset : "); Serial.println(gx, 6);
  Serial.print("Gyro Y Offset : "); Serial.println(gy, 6);
  Serial.print("Gyro Z Offset : "); Serial.println(gz, 6);
  Serial.println("=================================");
}

void loop() {
}