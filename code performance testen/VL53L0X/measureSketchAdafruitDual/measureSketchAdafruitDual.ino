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
// All the measures will be stored in this array.
uint16_t measures_1[ARRAY_LENGTH];
// All the measures will be stored in this array.
uint16_t measures_2[ARRAY_LENGTH];
// ToF sensor can measure till 200 cm on max range.
uint16_t counted_measures_each_centimeter_1[MAX_RANGE_VL53L0X_IN_MM];
// ToF sensor can measure till 200 cm on max range.
uint16_t counted_measures_each_centimeter_2[MAX_RANGE_VL53L0X_IN_MM];
int amount_out_of_range_measurements = 0;

float calculate_SD_sensor1() {
    float sum = 0.0, mean, SD = 0.0;
    int i;
    for (i = 0; i < counter; ++i) {
      sum += measures_1[i];
    }
    mean = sum / counter;
    for (i = 0; i < counter; ++i) {
      SD += pow(measures_1[i] - mean, 2);
    }
    return sqrt(SD / counter);
}

float calculate_SD_sensor2() {
    float sum = 0.0, mean, SD = 0.0;
    int i;
    for (i = 0; i < counter; ++i) {
      sum += measures_2[i];
    }
    mean = sum / counter;
    for (i = 0; i < counter; ++i) {
      SD += pow(measures_2[i] - mean, 2);
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


uint16_t calculate_median_sensor1() {
  // qsort - last parameter is a function pointer to the sort function
  quickSort(measures_1, 0, counter - 1);
  return measures_1[int(counter / 2)];
}

uint16_t calculate_median_sensor2() {
  // qsort - last parameter is a function pointer to the sort function
  quickSort(measures_2, 0, counter - 1);
  return measures_2[int(counter / 2)];
}

int calculate_avarage_sensor1() {
  int sum;
  for(int i = 0; i < counter; i++) {
    sum += measures_1[i]; 
  }
  return sum / counter;
}

int calculate_avarage_sensor2() {
  int sum;
  for(int i = 0; i < counter; i++) {
    sum += measures_2[i]; 
  }
  return sum / counter;
}

void print_result() {
  if(is_array_full)
    Serial.println("The array went full.");
  
  if(is_time_up)
    Serial.println("Measure time is done.");
  
  Serial.print("Amount of in range measurements taken with sensor 1:");
  Serial.print(counter);
  Serial.println();
  Serial.print("Amount of measurements that where measured out of range with sensor 1 :");
  Serial.print(amount_out_of_range_measurements);
  Serial.println();
  
  Serial.print("Avarage in MM from sensor 1: ");
  Serial.print(calculate_avarage_sensor1());
  Serial.println();
  // Necceray for now because the median function is sorting the array for the common functions...
  Serial.print("Median in MM from sensor 1: ");
  Serial.print(calculate_median_sensor1());
  Serial.println();
  
  Serial.print("Standard deviation in MM from sensor 1: ");
  Serial.print(calculate_SD_sensor1());
  Serial.println();

  Serial.print("Shortest measure in MM from sensor 1: ");
  Serial.print(measures_1[0]);
  Serial.println();

  Serial.print("Longest measure in MM from sensor 1: ");
  Serial.print(measures_1[counter - 1]);
  Serial.println();

  Serial.print("Difference longest and shortest of measure in MM from sensor 1: ");
  Serial.print(measures_1[counter - 1] - measures_1[0]);
  Serial.println();

  Serial.println("Measurements taken from sensor 1: ");
  for(int i = 0; i < MAX_RANGE_VL53L0X_IN_MM; i++) {
      if(counted_measures_each_centimeter_1[i] != 0) {

        Serial.print(counted_measures_each_centimeter_1[i]);
        Serial.print(" times a measure of ");
        Serial.print(i);
        Serial.print(" mm has been measured.");
        Serial.println();
      }
  }

  Serial.println("----------------------------");
  
  Serial.print("Avarage in MM from sensor 2: ");
  Serial.print(calculate_avarage_sensor2());
  Serial.println();
  // Necceray for now because the median function is sorting the array for the common functions...
  Serial.print("Median in MM from sensor 2: ");
  Serial.print(calculate_median_sensor2());
  Serial.println();
  
  Serial.print("Standard deviation in MM from sensor 2: ");
  Serial.print(calculate_SD_sensor2());
  Serial.println();
  
  Serial.print("Shortest measure in MM from sensor 2: ");
  Serial.print(measures_2[0]);
  Serial.println();
  
  Serial.print("Longest measure in MM from sensor 2: ");
  Serial.print(measures_2[counter - 1]);
  Serial.println();
  
  Serial.print("Difference longest and shortest of measure in MM from sensor 2: ");
  Serial.print(measures_2[counter - 1] - measures_2[0]);
  Serial.println();
  
  Serial.println("Measurements taken from sensor 2: ");
  for(int i = 0; i < MAX_RANGE_VL53L0X_IN_MM; i++) {
      if(counted_measures_each_centimeter_2[i] != 0) {

        Serial.print(counted_measures_each_centimeter_2[i]);
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

  
  if(is_startup_done && ! is_finished) {
      // Check if the performance test time is over.
      is_time_up = (millis() / 1000) > seconds_to_measure;
      // Check if the array went full.
      is_array_full = counter >= ARRAY_LENGTH;

      // If performance test time is over or the array went full.
      if(is_time_up || is_array_full) {
        is_finished = true;
        print_result();
      } else {

          // Check if one of the measurements ain't bullshit. Check if it is below the 120 cm.
          if(measure1.RangeMilliMeter < 1200 && measure2.RangeMilliMeter < 1200) {
            // Measuring with the ToF sensor, now we save the results.
            Serial.print("Range measurement of sensor 1 in mm: ");
            Serial.println(measure1.RangeMilliMeter);
            Serial.print("Range measurement of sensor 2 in mm: ");
            Serial.println(measure2.RangeMilliMeter);
            Serial.print(" Time left for measuring : ");
            Serial.print(seconds_to_measure - millis() / 1000);
            Serial.println();
            measures_1[counter] = measure1.RangeMilliMeter;
            measures_2[counter] = measure2.RangeMilliMeter;
            // Index represented the distance in cm, the value is the counted measurement of the distance in cm.
            counted_measures_each_centimeter_1[measure1.RangeMilliMeter] += 1;
            counted_measures_each_centimeter_2[measure2.RangeMilliMeter] += 1;
            Serial.print("Range status of sensor 1: ");
            Serial.println(measure1.RangeStatus);
            Serial.print("Range status of sensor 2: ");
            Serial.println(measure2.RangeStatus);
            counter++;
          } else {
            Serial.print("Out of range measurement has been registered");
            amount_out_of_range_measurements++;
          }
      }
  } else {
        if(!is_startup_done) {
          Serial.print("Seconds before starting: ");
          Serial.println(seconds_for_starting_up - (millis() / 1000));
          Serial.print("Range measurement of sensor 1 in mm: ");
          Serial.println(measure1.RangeMilliMeter);
          Serial.print("Range measurement of sensor 2 in mm: ");
          Serial.println(measure2.RangeMilliMeter);
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
  memset(&counted_measures_each_centimeter_1, 0x00, MAX_RANGE_VL53L0X_IN_MM * sizeof(counted_measures_each_centimeter_1[0]));
}

void loop() {
   update(); 
//  read_dual_sensors();
}
