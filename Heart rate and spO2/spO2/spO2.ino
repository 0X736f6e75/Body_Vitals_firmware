/*
 * Extracted Code for SpO2 Calculation Only
 *
 * This code includes:
 * 1. Required libraries (Wire, MAX30105, spo2_algorithm)
 * 2. Sensor initialization
 * 3. The specific sensor configuration required for the SpO2 algorithm
 * 4. The buffer (Red/IR) collection logic
 * 5. The "sliding window" buffer management
 * 6. The core function call to 'maxim_heart_rate_and_oxygen_saturation'
 * 7. Printing *only* the SpO2 result
 */

#include <Wire.h>
#include "MAX30105.h"
#include "spo2_algorithm.h"

MAX30105 particleSensor;

// --- Globals required for the SpO2 algorithm ---
uint32_t irBuffer[100];   // Infrared LED sensor data
uint32_t redBuffer[100];  // Red LED sensor data

int32_t bufferLength;     // Data length
int32_t spo2;             // SPO2 value
int8_t validSPO2;         // Indicator to show if the SPO2 calculation is valid
int32_t heartRate;        // Heart rate value (Required by the function, even if not displayed)
int8_t validHeartRate;    // (Required by the function)
// ------------------------------------------------

void setup() {
  Serial.begin(9600);
  Serial.println(F("Initializing SpO2 sensor..."));

  // Initialize sensor
  if (!particleSensor.begin(Wire, I2C_SPEED_FAST)) {
    Serial.println(F("MAX30105 was not found. Please check wiring/power."));
    while (1);
  }

  Serial.println(F("Place your finger on the sensor. Reading will start in 4 seconds..."));

  // --- Sensor Configuration (CRITICAL for SpO2 algorithm) ---
  // These settings are tuned for the 'spo2_algorithm.h' library
  byte ledBrightness = 60;
  byte sampleAverage = 4;
  byte ledMode = 2;          // Must be 2 (Red + IR) for SpO2
  byte sampleRate = 100;
  int pulseWidth = 411;
  int adcRange = 4096;

  particleSensor.setup(ledBrightness, sampleAverage, ledMode, sampleRate, pulseWidth, adcRange);

}

void loop() {
  bufferLength = 100; // Buffer length of 100 stores 4 seconds of samples

  // Read the first 100 samples to fill the buffer
  for (byte i = 0; i < bufferLength; i++) {
    while (particleSensor.available() == false)
      particleSensor.check();

    redBuffer[i] = particleSensor.getRed();
    irBuffer[i] = particleSensor.getIR();
    particleSensor.nextSample();
  }

  // Calculate heart rate and SpO2 after first 100 samples
  maxim_heart_rate_and_oxygen_saturation(irBuffer, bufferLength, redBuffer, &spo2, &validSPO2, &heartRate, &validHeartRate);

  // Continuously take samples and calculate SpO2
  while (1) {
    // --- Sliding Window Logic ---
    // Dump the first 25 samples
    for (byte i = 25; i < 100; i++) {
      redBuffer[i - 25] = redBuffer[i];
      irBuffer[i - 25] = irBuffer[i];
    }

    // Take 25 new samples
    for (byte i = 75; i < 100; i++) {
      while (particleSensor.available() == false)
        particleSensor.check();

      redBuffer[i] = particleSensor.getRed();
      irBuffer[i] = particleSensor.getIR();
      particleSensor.nextSample();
    }

    // --- Core Calculation ---
    // After gathering 25 new samples, recalculate
    maxim_heart_rate_and_oxygen_saturation(irBuffer, bufferLength, redBuffer, &spo2, &validSPO2, &heartRate, &validHeartRate);


    // --- Display SpO2 Result ---
    Serial.print(F("SpO2:"));
    Serial.print(spo2);
    Serial.print(F(" (Valid:"));
    Serial.print(validSPO2);
    Serial.print(F(")"));

    // Simple check: If the SpO2 is not valid, it likely means no finger is present
    if (validSPO2 == 0) {
        Serial.print(F(" --- No Finger? ---"));
    }

    Serial.println();
  }
}