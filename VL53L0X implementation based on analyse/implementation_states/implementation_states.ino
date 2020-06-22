#include "Adafruit_VL53L0X.h"
// amount of time for registering is (10 x (40 ms sensor time x two sensors)) = 800 ms
#define MAX_COUNTER_VALUE 10

// address we will assign if dual sensor is present
#define LOX1_ADDRESS 0x30
#define LOX2_ADDRESS 0x31

// set the pins to shutdown
#define SHT_LOX1 7
#define SHT_LOX2 6

// objects for the vl53l0x
Adafruit_VL53L0X lox1 = Adafruit_VL53L0X();
Adafruit_VL53L0X lox2 = Adafruit_VL53L0X();

// this holds the measurement
VL53L0X_RangingMeasurementData_t measure1;
VL53L0X_RangingMeasurementData_t measure2;

class State {
  protected:
    bool is_state_done;
  uint8_t counter;
  public:
    State() : is_state_done(false), counter(0) {
    }
    virtual void update(uint16_t measured_value_mm_left_sensor, uint16_t measured_value_mm_right_sensor);
    
    bool get_is_state_done() const {
      return is_state_done;
    }

};

class Food_presented : public State {
  public:
    void update(uint16_t measured_value_mm_left_sensor, uint16_t measured_value_mm_right_sensor) override {
      // Calculate the avarage from the two sensor measurements.
      uint16_t final_measure_mm = (measured_value_mm_left_sensor + measured_value_mm_right_sensor) / 2;
      
      // When the avarage, is not fitting in the current state, check if on the two sensor values still are.
      if(final_measure_mm <= 360 || final_measure_mm >= 800) {
        // If sensor measurement value left is still fitting in the current state, we take this as the final measure.
        if(measured_value_mm_left_sensor >= 360 && measured_value_mm_left_sensor <= 800) {
          final_measure_mm = measured_value_mm_left_sensor;
        }
        // If sensor measurement value right is still fitting in the current state, we take this as the final measure.
        if(measured_value_mm_right_sensor >= 360 && measured_value_mm_right_sensor <= 800) {
          final_measure_mm = measured_value_mm_right_sensor;
        }
      }
          
      // Check if the measured value is fitting in the current state.
      if(final_measure_mm <= 360 || final_measure_mm >= 800) {
        counter++;
      } else {
        counter = 0;
      }
      
      if(counter >= MAX_COUNTER_VALUE) {
        is_state_done = true;
        counter = 0;
      }
    }
};

class Idle : public State {
  public:
    void update(uint16_t measured_value_mm_left_sensor, uint16_t measured_value_mm_right_sensor) override {
      // Calculate the avarage from the two sensor measurements.
      uint16_t final_measure_mm = (measured_value_mm_left_sensor + measured_value_mm_right_sensor) / 2;
      
      // When the avarage, is not fitting in the current state, check if on the two sensor values still are.
      if(final_measure_mm >= 360 && final_measure_mm <= 800) {
        // If sensor measurement value left is still fitting in the current state, we take this as the final measure.
        if(measured_value_mm_left_sensor <= 360 || measured_value_mm_left_sensor >= 800) {
          final_measure_mm = measured_value_mm_left_sensor;
        }
        // If sensor measurement value right is still fitting in the current state, we take this as the final measure.
        if(measured_value_mm_right_sensor <= 360 || measured_value_mm_right_sensor >= 800) {
          final_measure_mm = measured_value_mm_right_sensor;
        }
      }
      
      if(final_measure_mm >= 360 && final_measure_mm <= 800) {
        counter++;
      } else {
        counter = 0;
      }
      
      if(counter >= MAX_COUNTER_VALUE) {
        is_state_done = true;
        counter = 0;
      }
    }
};

Idle idle;
Food_presented food_presented;
State* state;
bool is_state_idle = true;


void setup() {
  state = &idle;
}

void loop() {
  lox1.rangingTest(&measure1, false); // pass in 'true' to get debug data printout!
  lox2.rangingTest(&measure2, false); // pass in 'true' to get debug data printout!
  
  uint16_t measured_value_mm_sensor_left = measure1.RangeMilliMeter;
  uint16_t measured_value_mm_sensor_right = measure2.RangeMilliMeter;
  
  state->update(measured_value_mm_sensor_left, measured_value_mm_sensor_right);

  if(state->get_is_state_done()) {
    if(is_state_idle) {
      state = &food_presented;
    } else {
      state = &idle;
    }
    is_state_idle = !is_state_idle;
  }

  if(is_state_idle) {
    Serial.println("Current state is Idle.");  
  } else {
    Serial.println("Current state is Food presented.");
  }
}
