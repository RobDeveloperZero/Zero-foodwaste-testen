#include "Adafruit_VL53L0X.h"
// link to the api manual : https://www.st.com/resource/en/user_manual/dm00279088-world-smallest-timeofflight-ranging-and-gesture-detection-sensor-application-programming-interface-stmicroelectronics.pdf

// array length is 5000 for now, when I tried to make it more, i got a memory error.
#define ARRAY_LENGTH 4000
// max range for the vl53l0x ToF Sensor.
#define MAX_RANGE_VL53L0X_IN_MM 2000
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
/*
    Reset all sensors by setting all of their XSHUT pins low for delay(10), then set all XSHUT high to bring out of reset
    Keep sensor #1 awake by keeping XSHUT pin high
    Put all other sensors into shutdown by pulling XSHUT pins low
    Initialize sensor #1 with lox.begin(new_i2c_address) Pick any number but 0x29 and it must be under 0x7F. Going with 0x30 to 0x3F is probably OK.
    Keep sensor #1 awake, and now bring sensor #2 out of reset by setting its XSHUT pin high.
    Initialize sensor #2 with lox.begin(new_i2c_address) Pick any number but 0x29 and whatever you set the first sensor to
 */

bool is_finished, is_array_full, is_time_up = false;
// Amount of seconds to run the system before saving the measuring results.
int seconds_for_starting_up = 60;
// Amount of seconds for testing and saving the measuring results.
int seconds_for_testing = 300;
int seconds_to_measure = seconds_for_starting_up + seconds_for_testing;
// Counter for the amount of measures that are taken.
int counter = 0;
int counter_left = 0;
// All the measures will be stored in this array.
uint16_t measures_in_mm[ARRAY_LENGTH];
uint16_t measures_in_mm_left[ARRAY_LENGTH];
// ToF sensor can measure till 200 cm on max range.
uint16_t counted_measures_each_mm[MAX_RANGE_VL53L0X_IN_MM];
uint16_t counted_measures_each_mm_left[MAX_RANGE_VL53L0X_IN_MM];
int out_of_scope_measurements = 0;
int out_of_scope_measurements_left = 0;

float calculate_SD_sensor(uint16_t *measures, int counter) {
    float sum = 0.0, mean, SD = 0.0;
    int i;
    for (i = 0; i < counter; ++i) {
      sum += measures[i];
    }
    mean = sum / counter;
    for (i = 0; i < counter; ++i) {
      SD += pow(measures[i] - mean, 2);
    }
    return sqrt(SD / counter);
}


  
// A utility function to swap two elements  
void swap(uint16_t* a, uint16_t* b)  
{  
    uint16_t t = *a;  
    *a = *b;  
    *b = t;  
}  
  
/* This function takes last element as pivot, places  
the pivot element at its correct position in sorted  
array, and places all smaller (smaller than pivot)  
to left of pivot and all greater elements to right  
of pivot */
int partition (uint16_t arr[], int low, int high)  
{  
    int pivot = arr[high]; // pivot  
    int i = (low - 1); // Index of smaller element  
  
    for (int j = low; j <= high - 1; j++)  
    {  
        // If current element is smaller than the pivot  
        if (arr[j] < pivot)  
        {  
            i++; // increment index of smaller element  
            swap(&arr[i], &arr[j]);  
        }  
    }  
    swap(&arr[i + 1], &arr[high]);  
    return (i + 1);  
}  
  
/* The main function that implements QuickSort  
arr[] --> Array to be sorted,  
low --> Starting index,  
high --> Ending index */
void quickSort(uint16_t arr[], int low, int high)  
{  
    if (low < high)  
    {  
        /* pi is partitioning index, arr[p] is now  
        at right place */
        int pi = partition(arr, low, high);  
  
        // Separately sort elements before  
        // partition and after partition  
        quickSort(arr, low, pi - 1);  
        quickSort(arr, pi + 1, high);  
    }  
}  


uint16_t calculate_median_sensor(uint16_t *measures, int counter) {
  // qsort - last parameter is a function pointer to the sort function
  quickSort(measures, 0, counter - 1);
  return measures[int(counter / 2)];
}


uint16_t calculate_avarage_sensor(uint16_t *measures, int counter) {
  int sum;
  for(int i = 0; i < counter; i++) {
    sum += measures[i]; 
  }
  return sum / counter;
}

uint16_t measurementCalculation(VL53L0X_RangingMeasurementData_t *measure1, VL53L0X_RangingMeasurementData_t *measure2) {
  VL53L0X_RangingMeasurementData_t *measureObjects[2];
  measureObjects[0] = measure1;
  measureObjects[1] = measure2;
  
  // Array to story if an measurement was out of range.
  bool is_out_of_range[2] = {false, false};
  
  // Check if a sensor measurement is out of range.
  for(int i = 0; i < 2; i++) {
    if(measureObjects[i]->RangeMilliMeter <= 360 && measureObjects[i]->RangeMilliMeter >= 800) {
      is_out_of_range[i] = true;
    } else {
      is_out_of_range[i] = false;  
    }
  }
  
  uint16_t measurement = (measureObjects[0]->RangeMilliMeter + measureObjects[1]->RangeMilliMeter) / 2;
  
  if(is_out_of_range[0] && is_out_of_range[1]) {
    measurement = 0;
  } else if(is_out_of_range[0] && !is_out_of_range[1]) {
    measurement = measureObjects[1]->RangeMilliMeter;
  } else if(is_out_of_range[1] && !is_out_of_range[0]) {
    measurement = measureObjects[0]->RangeMilliMeter;
  }

  return measurement;
}

void print_result() {
  if(is_array_full)
    Serial.println("The array went full.");
  
  if(is_time_up)
    Serial.println("Measure time is done.");
  Serial.println("==*==*==*==*==*==*==*==*==*==*==*==*==*");
  Serial.print("Measurements in range:");
  Serial.print(counter);
  Serial.println();
  Serial.print("Amount of measurements that where measured out of range :");
  Serial.print(out_of_scope_measurements);
  Serial.println();
  Serial.println();
  Serial.print("Measurement avarage in MM: ");
  Serial.print(calculate_avarage_sensor(measures_in_mm, counter));
  Serial.println();
  // Necceray for now because the median function is sorting the array for the common functions...
  Serial.print("Median in MM: ");
  Serial.print(calculate_median_sensor(measures_in_mm, counter));
  Serial.println();
  
  Serial.print("Standard deviation in MM: ");
  Serial.print(calculate_SD_sensor(measures_in_mm, counter));
  Serial.println();

  Serial.print("Shortest measure in MM: ");
  Serial.print(measures_in_mm[0]);
  Serial.println();

  Serial.print("Longest measure in MM: ");
  Serial.print(measures_in_mm[counter - 1]);
  Serial.println();

  Serial.print("Difference longest and shortest of measure in MM: ");
  Serial.print(measures_in_mm[counter - 1] - measures_in_mm[0]);
  Serial.println();

  Serial.println("Measurements sorted and counted: ");
  for(int i = 0; i < MAX_RANGE_VL53L0X_IN_MM; i++) {
      if(counted_measures_each_mm[i] != 0) {
        Serial.print(counted_measures_each_mm[i]);
        Serial.print(" times a measure of ");
        Serial.print(i);
        Serial.print(" mm has been measured.");
        Serial.println();
      }
  }
  Serial.println("==*==*==*==*==*==*==*==*==*==*==*==*==*");
  Serial.print("Measurements in range:");
  Serial.print(counter_left);
  Serial.println();
  Serial.print("Amount of measurements that where measured out of range with left sensor only :");
  Serial.print(out_of_scope_measurements_left);
  Serial.println();
  Serial.print("Measurement avarage in MM with left sensor only: ");
  Serial.print(calculate_avarage_sensor(measures_in_mm_left, counter_left));
  Serial.println();
  // Necceray for now because the median function is sorting the array for the common functions...
  Serial.print("Median in MM with left sensor only: ");
  Serial.print(calculate_median_sensor(measures_in_mm_left, counter_left));
  Serial.println();
  
  Serial.print("Standard deviation in MM with left sensor only: ");
  Serial.print(calculate_SD_sensor(measures_in_mm_left, counter_left));
  Serial.println();

  Serial.print("Shortest measure in MM with left sensor only: ");
  Serial.print(measures_in_mm_left[0]);
  Serial.println();

  Serial.print("Longest measure in MM with left sensor only: ");
  Serial.print(measures_in_mm_left[counter_left - 1]);
  Serial.println();

  Serial.print("Difference longest and shortest of measure in MM with left sensor only: ");
  Serial.print(measures_in_mm_left[counter_left - 1] - measures_in_mm_left[0]);
  Serial.println();

  Serial.println("Measurements sorted and counted with left sensor only: ");
  for(int i = 0; i < MAX_RANGE_VL53L0X_IN_MM; i++) {
      if(counted_measures_each_mm_left[i] != 0) {
        Serial.print(counted_measures_each_mm_left[i]);
        Serial.print(" times a measure of ");
        Serial.print(i);
        Serial.print(" mm has been measured.");
        Serial.println();
      }
  }
}

void update() {
  // Check if startup time is over.
  bool is_startup_done = (millis() / 1000) > seconds_for_starting_up;
  lox1.rangingTest(&measure1, false); // pass in 'true' to get debug data printout!
  lox2.rangingTest(&measure2, false); // pass in 'true' to get debug data printout!
  uint16_t measurement_in_mm = measurementCalculation(&measure1, &measure2);
  if(is_startup_done && ! is_finished) {
      // Check if the performance test time is over.
      is_time_up = (millis() / 1000) > seconds_to_measure;
      // Check if the array went full.
      is_array_full = counter >= ARRAY_LENGTH || counter >= ARRAY_LENGTH;
      
      // If performance test time is over or the array went full.
      if(is_time_up || is_array_full) {
        is_finished = true;
        print_result();
      } else {
        Serial.print("Calculated measurement from both sensors: ");
        Serial.println(measurement_in_mm);
        if(measurement_in_mm != 0) {
          measures_in_mm[counter++] = measurement_in_mm;
          counted_measures_each_mm[measurement_in_mm]++;
        } else {
          out_of_scope_measurements++;  
        }

         Serial.print("Measurement from left sensors only: ");
        Serial.println(measure1.RangeMilliMeter);
        if(measure1.RangeMilliMeter >= 440 && measure1.RangeMilliMeter <= 560) {
          measures_in_mm_left[counter_left++] = measure1.RangeMilliMeter;
          counted_measures_each_mm_left[measure1.RangeMilliMeter]++;
        } else {
          out_of_scope_measurements_left++;  
        }
        
        Serial.print(" Time left for measuring : ");
        Serial.print(seconds_to_measure - millis() / 1000);
        Serial.println();  
      }
  } else {
        if(!is_startup_done) {
          Serial.print("Seconds before starting: ");
          Serial.println(seconds_for_starting_up - (millis() / 1000));
          Serial.print("Calculated measurement from both sensors: ");
          Serial.println(measurement_in_mm);
          Serial.print("Measurement from left sensors only: ");
          Serial.println(measure1.RangeMilliMeter);
          Serial.println();
        } 
  }
}

void setID() {
  // all reset
  digitalWrite(SHT_LOX1, LOW); 
  digitalWrite(SHT_LOX2, LOW);   
  delay(10);
  // all unreset
  digitalWrite(SHT_LOX1, HIGH);
  digitalWrite(SHT_LOX2, HIGH);
  delay(10);

  // activating LOX1 and reseting LOX2
  digitalWrite(SHT_LOX1, HIGH);
  digitalWrite(SHT_LOX2, LOW);

  // initing LOX1
  if(!lox1.begin(LOX1_ADDRESS)) {
    Serial.println(F("Failed to boot first VL53L0X"));
    while(1);
  }
  delay(10);

  // activating LOX2
  digitalWrite(SHT_LOX2, HIGH);
  delay(10);

  //initing LOX2  
  if(!lox2.begin(LOX2_ADDRESS)) {
    Serial.println(F("Failed to boot second VL53L0X"));
    while(1);
  }
}

void setup() {
  Serial.begin(115200);
  // wait until serial port opens for native USB devices
  while (! Serial) { delay(1); }

  pinMode(SHT_LOX1, OUTPUT);
  pinMode(SHT_LOX2, OUTPUT);

  Serial.println("Shutdown pins inited...");

  digitalWrite(SHT_LOX1, LOW);
  digitalWrite(SHT_LOX2, LOW);

  Serial.println("Both in reset mode...(pins are low)");
  
  Serial.println("Starting...");
  setID();

  // Make sure all values are set to 0 in the array.
  memset(&counted_measures_each_mm, 0x00, MAX_RANGE_VL53L0X_IN_MM * sizeof(counted_measures_each_mm[0]));
}

void loop() {
   update(); 
//  read_dual_sensors();
}
