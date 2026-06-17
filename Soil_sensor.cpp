#include <Arduino.h>

class CapacitiveSensor {
private:
    int sendPin;
    int receivePin;

public:
    // Constructor to initialize pins
    CapacitiveSensor(int send_pin, int receive_pin) {
        sendPin = send_pin;
        receivePin = receive_pin;
        
        pinMode(sendPin, OUTPUT);
        pinMode(receivePin, INPUT);
        digitalWrite(sendPin, LOW);
    }

    // Measures the capacitance based on RC time constant
    long read(unsigned int samples) {
        long totalCycleCount = 0;

        for (unsigned int i = 0; i < samples; i++) {
            // 1. Discharge the capacitor (human body/pad)
            digitalWrite(sendPin, LOW);
            pinMode(receivePin, OUTPUT);
            digitalWrite(receivePin, LOW);
            delayMicroseconds(10);
            
            // 2. Set receive pin back to input
            pinMode(receivePin, INPUT);
            
            // 3. Start timing and pull send pin HIGH
            digitalWrite(sendPin, HIGH);
            
            long cycles = 0;
            // Count how long it takes for the receive pin to go HIGH
            while (digitalRead(receivePin) == LOW) {
                cycles++;
                // Timeout to prevent infinite loops if disconnected
                if (cycles > 10000) break; 
            }
            
            totalCycleCount += cycles;
        }
        
        return totalCycleCount / samples;
    }
};

// Hardware configuration
const int SEND_PIN = 4;
const int RECEIVE_PIN = 2;
const int TOUCH_THRESHOLD = 50; // Adjust based on your resistor value

// Create sensor instance
CapacitiveSensor touchSensor(SEND_PIN, RECEIVE_PIN);

void setup() {
    Serial.begin(9600);
}

void loop() {
    // Read sensor average over 10 samples
    long sensorValue = touchSensor.read(10);
    
    Serial.print("Sensor Value: ");
    Serial.print(sensorValue);

    if (sensorValue > TOUCH_THRESHOLD) {
        Serial.println(" -> [TOUCHED]");
    } else {
        Serial.println(" -> [RELEASED]");
    }

    delay(100);
}
