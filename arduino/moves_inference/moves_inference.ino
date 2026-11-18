/*
  moves_inference.ino
  Runs your Edge Impulse model on the Nano and prints one line over USB
  serial every INFER_EVERY_MS milliseconds, in the form:

      label,confidence        for example:   MOVE_1,0.62

  The Processing sketch (moves_display.pde) reads these lines.

  Board:  Arduino Nano 33 BLE Sense Rev2 (BMI270 / BMM150 IMU)
          Original board (LSM9DS1)? Change the IMU include below to
          #include <Arduino_LSM9DS1.h>

  SETUP
  1. Edge Impulse: Deployment > Arduino library > Build, download the .zip.
  2. Arduino IDE: Sketch > Include Library > Add .ZIP Library.
  3. Change the first #include below to YOUR library's header name.
     It ends in _inferencing.h (Sketch > Include Library lists it).
  4. Upload. To check it, open the Serial Monitor at 115200.
  5. CLOSE the Serial Monitor before you run the Processing sketch,
     because only one program can use the USB port at a time.

  AXES
  This sketch works with either kind of impulse. It reads the number of
  axes from your exported library, so there is nothing to change:
    3 axes = gyroscope only      (gyrX, gyrY, gyrZ)
    6 axes = accelerometer + gyroscope (accX, accY, accZ, gyrX, gyrY, gyrZ)
  The order must match the order you named the axes when you recorded
  your data.

  Rebuild and re-add the library every time you retrain the model.
*/

#include <YOUR_PROJECT_NAME_inferencing.h>   // <-- TWEAK: your library's header name
#include <Arduino_BMI270_BMM150.h>           // <-- TWEAK only if your board is the original (LSM9DS1)

// ======================= TWEAK THESE =======================

// false = print "label,confidence" lines for the Processing sketch.
// true  = print every class's probability, so you can read the numbers
//         in the Serial Monitor when you set thresholds (ten takes).
#define DEBUG_ALL      false

// Time between answers, in milliseconds.
// Processing counts answers in a row, so:
//   HITS_NEEDED x INFER_EVERY_MS = how long you must hold a move.
// For example 3 hits x 250 ms = about 0.75 seconds.
#define INFER_EVERY_MS 250

// ===========================================================

#if (EI_CLASSIFIER_RAW_SAMPLES_PER_FRAME != 3) && (EI_CLASSIFIER_RAW_SAMPLES_PER_FRAME != 6)
#error "Your impulse should use 3 axes (gyro only) or 6 axes (accelerometer + gyro)."
#endif

const int AXES = EI_CLASSIFIER_RAW_SAMPLES_PER_FRAME;   // 3 or 6
const int N    = EI_CLASSIFIER_RAW_SAMPLE_COUNT;        // readings per window

static float ring[EI_CLASSIFIER_DSP_INPUT_FRAME_SIZE];      // circular buffer
static float features[EI_CLASSIFIER_DSP_INPUT_FRAME_SIZE];  // window handed to the model

const unsigned long intervalUs = (unsigned long)(EI_CLASSIFIER_INTERVAL_MS * 1000.0f);
unsigned long nextTick;
unsigned long lastInfer = 0;
int head  = 0;
int count = 0;
float ax = 0, ay = 0, az = 0, gx = 0, gy = 0, gz = 0;

void setup() {
  Serial.begin(115200);
  pinMode(LED_BUILTIN, OUTPUT);

  if (!IMU.begin()) {
    while (1) {                         // fast flashing light = motion sensor failed to start
      digitalWrite(LED_BUILTIN, HIGH); delay(100);
      digitalWrite(LED_BUILTIN, LOW);  delay(100);
    }
  }
  nextTick = micros();
}

void runInference() {
  // copy the circular buffer into a straight array, oldest reading first
  for (int i = 0; i < N; i++) {
    int idx = ((head + i) % N) * AXES;
    for (int k = 0; k < AXES; k++) {
      features[i * AXES + k] = ring[idx + k];
    }
  }

  signal_t signal;
  numpy::signal_from_buffer(features, EI_CLASSIFIER_DSP_INPUT_FRAME_SIZE, &signal);

  ei_impulse_result_t result;
  if (run_classifier(&signal, &result, false) != EI_IMPULSE_OK) return;

  // find the class with the highest probability
  int best = 0;
  for (size_t i = 1; i < EI_CLASSIFIER_LABEL_COUNT; i++) {
    if (result.classification[i].value > result.classification[best].value) best = i;
  }

#if DEBUG_ALL
  Serial.print(">> ");
  Serial.print(result.classification[best].label);
  Serial.print("  |  ");
  for (size_t i = 0; i < EI_CLASSIFIER_LABEL_COUNT; i++) {
    Serial.print(result.classification[i].label);
    Serial.print(' ');
    Serial.print(result.classification[i].value, 2);
    if (i < EI_CLASSIFIER_LABEL_COUNT - 1) Serial.print("  ");
  }
  Serial.println();
#else
  Serial.print(result.classification[best].label);
  Serial.print(',');
  Serial.println(result.classification[best].value, 2);
#endif
}

void loop() {
  // read the sensor at the impulse's rate and store it in the circular buffer
  unsigned long now = micros();
  if ((long)(now - nextTick) >= 0) {
    nextTick += intervalUs;

    if (IMU.accelerationAvailable()) IMU.readAcceleration(ax, ay, az);
    if (IMU.gyroscopeAvailable())    IMU.readGyroscope(gx, gy, gz);

    float sample[6] = { ax, ay, az, gx, gy, gz };
    int first = (AXES == 6) ? 0 : 3;     // 6 axes: start at accX. 3 axes: start at gyrX.
    for (int k = 0; k < AXES; k++) {
      ring[head * AXES + k] = sample[first + k];
    }
    head = (head + 1) % N;
    if (count < N) count++;
  }

  // classify on a fixed interval once the buffer is full
  if (count >= N && millis() - lastInfer >= INFER_EVERY_MS) {
    lastInfer = millis();
    runInference();
  }
}
