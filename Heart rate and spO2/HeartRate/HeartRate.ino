#include <Wire.h>
#include "MAX30105.h"
#include "heartRate.h"

MAX30105 particleSensor;

long lastBeat = 0; // Time at which the last beat occurred

// --- New variables for 10-second batching ---
const long readingInterval = 10000; // 10 seconds in milliseconds
long windowStartTime = 0;           // When the current 10s window started

const int MAX_BEATS_PER_WINDOW = 100;     // Max beats to store (60BPM * 20s = 20 beats, 180BPM = 60 beats. 100 is safe)
float beatReadings[MAX_BEATS_PER_WINDOW]; // Array to store all BPM readings in the window
int beatCount = 0;                        // Number of beats stored so far in this window
// --- End of new variables ---

// We no longer need RATE_SIZE, rates[], rateSpot, beatsPerMinute, or beatAvg as global variables.

void setup()
{
  Serial.begin(9600);
  Serial.println("Initializing...");

  // Initialize sensor
  if (!particleSensor.begin(Wire, I2C_SPEED_FAST))
  {
    Serial.println("MAX30102 was not found. Please check wiring/power. ");
    while (1)
      ;
  }
  Serial.println("Place your index finger on the sensor with steady pressure.");

  particleSensor.setup();                    // Configure sensor with default settings
  particleSensor.setPulseAmplitudeRed(0x0F); // Turn Red LED to low to indicate sensor is running
  particleSensor.setPulseAmplitudeGreen(0);  // Turn off Green LED

  // Start the first reading window
  Serial.println("\nStarting new 10-second reading window...");
  windowStartTime = millis();
}

void loop()
{
  long irValue = particleSensor.getIR();

  // --- Part 1: Collect data DURING the 10-second window ---
  if (millis() - windowStartTime < readingInterval)
  {

    if (checkForBeat(irValue) == true)
    {
      // We sensed a beat!
      long delta = millis() - lastBeat;
      lastBeat = millis();

      // Calculate the instantaneous BPM
      float beatsPerMinute = 60 / (delta / 1000.0);

      // Store this valid reading in our batch array
      if (beatsPerMinute < 255 && beatsPerMinute > 20)
      {
        // Check if we have space in the array
        if (beatCount < MAX_BEATS_PER_WINDOW)
        {
          beatReadings[beatCount] = beatsPerMinute;
          beatCount++;
        }
      }
    }
  }

  // --- Part 2: Calculate and Show Readings AFTER the 10-second window ---
  else
  {

    // 10 seconds are up! Time to calculate.
    Serial.println("---------------------------------");
    Serial.println("10-Second Window Finished.");

    if (beatCount > 0)
    {
      // Calculate the average
      double totalBPM = 0; // Use a double for accuracy while summing
      for (int i = 0; i < beatCount; i++)
      {
        totalBPM += beatReadings[i];
      }
      float averageBPM = totalBPM / (double)beatCount;

      // Show the final readings
      Serial.print("Average BPM: ");
      Serial.println(averageBPM);
      Serial.print("Total Beats Detected: ");
      Serial.println(beatCount);
    }
    else
    {
      // We found no beats in 10 seconds
      Serial.println("No valid beats detected in this window.");
    }

    // Also, print a finger check
    if (irValue < 50000)
    {
      Serial.println("Signal quality is low. (No finger?)");
    }

    Serial.println("---------------------------------\n");

    // --- Part 3: Reset for the next window ---
    beatCount = 0; // Clear the counter
    // Note: We don't need to clear the old array data, we'll just overwrite it

    windowStartTime = millis(); // Start the new 20-second window
    Serial.println("Starting new 10-second reading window...");
  }
}