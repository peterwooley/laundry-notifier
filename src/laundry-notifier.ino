/*
 * Project laundry-notifier
 * Description: Auto-sense when the dryer starts and send a notification when it starts and finishes
 * Author: Peter Wooley <peterwooley@gmail.com>
 * Date: 2018-08-19
 */

/*
## States
  * OFF
  * STARTING
  * STOPPING
  * ENDING

## Transitions
  * OFF -> STARTING
    * When a vibration ≥ VIBRATION_THRESHOLD is sensed
    * Store startingTime
  * STARTING -> OFF
    * When VIBRATION_THRESHOLD is sampled < RUNNING_THRESHOLD% since changing states
    * Reset startingTime
  * STARTING -> RUNNING
    * When VIBRATION_THRESHOLD is sampled ≥ RUNNING_THRESHOLD% since changing states
  * RUNNING -> STOPPING
    * When a vibration < VIBRATION_THRESHOLD is sensed
    * Store stoppingTime
  * STOPPING -> RUNNING
    * When VIBRATION_THRESHOLD is sampled < RUNNING_THRESHOLD% since changing states
    * Reset stoppingTime
  * STOPPING -> OFF
    * When VIBRATION_THRESHOLD is sampled ≥ RUNNING_THRESHOLD% since changing states
    * Send notification
    * Reset startingTime
    * Reset stoppingTime
 */

const int PIEZO_PIN = A0;

enum state {
  OFF,
  STARTING,
  RUNNING,
  STOPPING
};

enum state machineState = OFF;
const float VIBRATION_THRESHOLD = 7;
const float RUNNING_VIBRATION_THRESHOLD = 0.9;
const float RUNNING_TIME_THRESHOLD = 10000;

// Smoothing
const int numReadings = 10;
int readings [numReadings];
int readIndex = 0;
int biggest;
int smallest;
int average = 0;

unsigned long startingTime;
unsigned long stoppingTime;
unsigned long elapsedTime;
int totalReadings = 0;
int aboveThresholdReadings = 0;
int belowThresholdReadings = 0;
float readingRatio;

void setup() {
  //Serial.begin(9600);
  RGB.control(true);
  for (int i = 0; i < numReadings; i++) {
    readings[i] = 0;
  }
}

void loop() {
  float piezoVibration = analogRead(PIEZO_PIN);
  //Serial.println(piezoVibration);
  //Particle.publish("vibration-detected", String(piezoVibration));

  // Smooth readings
  readings[readIndex] = piezoVibration;

  smallest = 1000;
  biggest = 0;
  for (int i = 0; i < numReadings; i++) {
    if(readings[i] < smallest) {
      smallest = readings[i];
    }
    if(readings[i] > biggest) {
      biggest = readings[i];
    }
  }

  readIndex = readIndex + 1;
  if (readIndex >= numReadings) {
    readIndex = 0;
  }
  int vibrationSpread = biggest-smallest;
  //Serial.println("Spread: " + String(vibrationSpread));
  //Particle.publish("vibration-spread", String(vibrationSpread));

  switch(machineState) {
    case OFF:
      RGB.color(255, 255, 255);
      RGB.brightness(50);
      if(vibrationSpread >= VIBRATION_THRESHOLD) {
        // Transition OFF -> STARTING
        startingTime = millis();
        machineState = STARTING;
        //Serial.println(machineState);
      }
      break;
    case STARTING:
      RGB.color(255, 255, 0);
      RGB.brightness(150);
      totalReadings++;
      if(vibrationSpread >= VIBRATION_THRESHOLD) {
        aboveThresholdReadings++;
      }

      readingRatio = (float) aboveThresholdReadings / (float) totalReadings;
      //Serial.println("Above Threshold Readings: " + String(readingRatio));

      if(readingRatio < RUNNING_VIBRATION_THRESHOLD) {
        // Transition STARTING -> OFF
        totalReadings = 0;
        aboveThresholdReadings = 0;
        startingTime = NULL;
        machineState = OFF;
        break;
      }

      elapsedTime = millis() - startingTime;
      //Serial.println("Starting elapsed time: " + String(elapsedTime));
      if(elapsedTime >= RUNNING_TIME_THRESHOLD) {
        // Transition STARTING -> RUNNING
        totalReadings = 0;
        aboveThresholdReadings = 0;
        machineState = RUNNING;

        Particle.publish("dryer-started");
      }

      break;
    case RUNNING:
      RGB.color(0, 255, 0);
      RGB.brightness(255);

      if(vibrationSpread < VIBRATION_THRESHOLD) {
        stoppingTime = millis();
        machineState = STOPPING;
        //Serial.println(machineState);
      }

      break;
    case STOPPING:
      RGB.color(255, 0, 0);
      RGB.brightness(255);

      totalReadings++;
      if(vibrationSpread < VIBRATION_THRESHOLD) {
        belowThresholdReadings++;
      }

      readingRatio = (float) belowThresholdReadings / (float) totalReadings;

      if(readingRatio < RUNNING_VIBRATION_THRESHOLD) {
        // Transition STARTING -> OFF
        totalReadings = 0;
        belowThresholdReadings = 0;
        stoppingTime = NULL;
        machineState = RUNNING;
        break;
      }

      elapsedTime = millis() - stoppingTime;
      //Serial.println("Stopping elapsed time: " + String(elapsedTime));
      if(elapsedTime >= RUNNING_TIME_THRESHOLD) {
        // Transition STARTING -> OFF
        totalReadings = 0;
        belowThresholdReadings = 0;
        machineState = OFF;

        Particle.publish("dryer-ended");
      }

      break;
  }
  delay(500);
}
