#include <Wire.h>
#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>
#include <KKM-Equipment-Classification_inferencing.h>

Adafruit_MPU6050 mpu;

/** Number sensor axes used */
#define N_SENSORS     3

// Define the data array in the global scope
float data[N_SENSORS];

// Forward declarations of the functions
bool init_IMU(void);
uint8_t poll_IMU(void);

/** Struct to link sensor axis name to sensor value function */
typedef struct {
  const char *name;
  float *value;
  uint8_t (*poll_sensor)(void);
  bool (*init_sensor)(void);
  int8_t status;  // -1 not used, 0 used (uninitialized), 1 used (initialized), 2 data sampled
} eiSensors;

/** Used sensors value function connected to label name */
eiSensors sensors[] = {
  "accX", &data[0], &poll_IMU, &init_IMU, -1,
  "accY", &data[1], &poll_IMU, &init_IMU, -1,
  "accZ", &data[2], &poll_IMU, &init_IMU, -1
};

bool init_IMU(void) {
  static bool init_status = false;
  if (!init_status) {
    init_status = mpu.begin();
    if (!init_status) {
      Serial.println("Failed to initialize MPU6050!");
      return false;
    }
  }
  return init_status;
}

uint8_t poll_IMU(void) {
  sensors_event_t a, g, temp;
  mpu.getEvent(&a, &g, &temp);

  data[0] = a.acceleration.x;
  data[1] = a.acceleration.y;
  data[2] = a.acceleration.z;

  return 0;
}

void setup() {
  Serial.begin(115200);
  Wire.begin();

  if (!init_IMU()) {
    while (1);
  }
}

void loop() {
  Serial.println("Starting inferencing...");

  delay(2000);

  float buffer[EI_CLASSIFIER_DSP_INPUT_FRAME_SIZE] = { 0 };

  for (size_t ix = 0; ix < EI_CLASSIFIER_DSP_INPUT_FRAME_SIZE; ix += EI_CLASSIFIER_RAW_SAMPLES_PER_FRAME) {
    int64_t next_tick = micros() + (EI_CLASSIFIER_INTERVAL_MS * 1000);

    poll_IMU();
    for (int i = 0; i < N_SENSORS; i++) {
      buffer[ix + i] = data[i];
    }

    delayMicroseconds(max(static_cast<int64_t>(0), next_tick - micros()));
  }

  signal_t signal;
  int err = numpy::signal_from_buffer(buffer, EI_CLASSIFIER_DSP_INPUT_FRAME_SIZE, &signal);
  if (err != 0) {
    Serial.println("Failed to create signal from buffer");
    return;
  }

  ei_impulse_result_t result;
  err = run_classifier(&signal, &result, false);
  if (err != EI_IMPULSE_OK) {
    Serial.print("Failed to run classifier (error: ");
    Serial.print(err);
    Serial.println(")");
    return;
  }

  Serial.println("Predictions:");
  for (size_t ix = 0; ix < EI_CLASSIFIER_LABEL_COUNT; ix++) {
    Serial.print(result.classification[ix].label);
    Serial.print(": ");
    Serial.println(result.classification[ix].value);
  }

  // Check if the model has anomaly detection
  #if EI_CLASSIFIER_HAS_ANOMALY == 1
    Serial.print("Anomaly score: ");
    Serial.println(result.anomaly);
    
    // You can add your custom threshold for anomaly detection
    float anomaly_threshold = 0.5;  // Example threshold
    if (result.anomaly > anomaly_threshold) {
      // Handle anomaly detected
      Serial.println("Anomaly detected!");
      // Add your code to handle the anomaly here
    }
  #endif
}