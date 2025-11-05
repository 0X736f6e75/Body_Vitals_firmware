/*
 * Combined code to read:
 * 1. SpO2 (Blood Oxygen)
 * 2. Heart Rate (BPM)
 * 3. Temperature (C)
 * 4. Red Light Value
 * 5. IR Light Value
 * 6. Presence Detection ("No Finger?")
 *
 * This code is a synthesis of the examples provided, primarily based on
 * Example 3 (SpO2) and augmented with features from Examples 2 and 4.
 */

#include <Wire.h>
#include "MAX30105.h"
#include "spo2_algorithm.h"

MAX30105 particleSensor;

// --- Globals from SpO2 Example ---
// Use 32-bit buffers for better compatibility with ESP32/modern boards
uint32_t irBuffer[100];   // Infrared LED sensor data
uint32_t redBuffer[100];  // Red LED sensor data

int32_t bufferLength;     // Data length
int32_t spo2;             // SPO2 value
int8_t validSPO2;         // Indicator to show if the SPO2 calculation is valid
int32_t heartRate;        // Heart rate value
int8_t validHeartRate;    // Indicator to show if the heart rate calculation is valid
// ---------------------------------

void setup() {
  Serial.begin(9600); // Or 115200 for faster data
  Serial.println(F("Initializing sensor..."));

  // Initialize sensor
  if (!particleSensor.begin(Wire, I2C_SPEED_FAST)) // Use default I2C port, 400kHz speed
  {
    Serial.println(F("MAX30105 was not found. Please check wiring/power."));
    while (1);
  }

  Serial.println(F("Place your finger on the sensor. Reading will start in 4 seconds..."));

  // --- Sensor Configuration (from SpO2 Example) ---
  byte ledBrightness = 60;   // Options: 0=Off to 255=50mA
  byte sampleAverage = 4;    // Options: 1, 2, 4, 8, 16, 32
  byte ledMode = 2;          // Options: 1 = Red only, 2 = Red + IR, 3 = Red + IR + Green
  byte sampleRate = 100;     // Options: 50, 100, 200, 400, 800, 1000, 1600, 3200
  int pulseWidth = 411;      // Options: 69, 118, 215, 411
  int adcRange = 4096;       // Options: 2048, 4096, 8192, 16384

  particleSensor.setup(ledBrightness, sampleAverage, ledMode, sampleRate, pulseWidth, adcRange);

  // --- Enable Temperature (from Temp Example) ---
  // This is required to read the temperature
  particleSensor.enableDIETEMPRDY();
}

void loop() {
  bufferLength = 100; // Buffer length of 100 stores 4 seconds of samples running at 25sps

  // Read the first 100 samples (first 4 seconds) to fill the buffer
  Serial.println(F("Reading initial samples..."));
  for (byte i = 0; i < bufferLength; i++) {
    while (particleSensor.available() == false) // Do we have new data?
      particleSensor.check(); // Check the sensor for new data

    redBuffer[i] = particleSensor.getRed();
    irBuffer[i] = particleSensor.getIR();
    particleSensor.nextSample(); // We're finished with this sample so move to next sample
  }

  // Calculate heart rate and SpO2 after first 100 samples
  maxim_heart_rate_and_oxygen_saturation(irBuffer, bufferLength, redBuffer, &spo2, &validSPO2, &heartRate, &validHeartRate);

  // Continuously take samples from MAX30105.
  // Heart rate and SpO2 are calculated every 1 second (25 new samples)
  while (1) {
    // Dumping the first 25 sets of samples in the memory and shifting the last 75 sets
    for (byte i = 25; i < 100; i++) {
      redBuffer[i - 25] = redBuffer[i];
      irBuffer[i - 25] = irBuffer[i];
    }

    // Take 25 new sets of samples
    for (byte i = 75; i < 100; i++) {
      while (particleSensor.available() == false) // Do we have new data?
        particleSensor.check(); // Check the sensor for new data

      redBuffer[i] = particleSensor.getRed();
      irBuffer[i] = particleSensor.getIR();
      particleSensor.nextSample(); // We're finished with this sample
    }

    // After gathering 25 new samples, recalculate HR and SP02
    maxim_heart_rate_and_oxygen_saturation(irBuffer, bufferLength, redBuffer, &spo2, &validSPO2, &heartRate, &validHeartRate);

    // --- Now, read and display ALL parameters ---

    // 1. Get Temperature
    float temperature = particleSensor.readTemperature(); // Read from Temp Example

    // 2. Get latest Red/IR values
    uint32_t currentIR = irBuffer[99];
    uint32_t currentRed = redBuffer[99];

    // 3. Get Presence Detection
    bool fingerDetected = (currentIR > 50000); // Logic from BPM Example

    // 4. Print all data in one clean line
    Serial.print(F("R:"));
    Serial.print(currentRed);

    Serial.print(F(", IR:"));
    Serial.print(currentIR);
    
    Serial.print(F(", HR:"));
    Serial.print(heartRate);
    Serial.print(F(" (Valid:"));
    Serial.print(validHeartRate);
    Serial.print(F(")"));

    Serial.print(F(", SpO2:"));
    Serial.print(spo2);
    Serial.print(F(" (Valid:"));
    Serial.print(validSPO2);
    Serial.print(F(")"));

    Serial.print(F(", TempC:"));
    Serial.print(temperature, 2); // Print with 2 decimal places

    if (!fingerDetected) {
      Serial.print(F(" --- NO FINGER ---"));
    }

    Serial.println(); // New line for next set of readings
  }
}