#include "Adafruit_VL53L0X.h"

// array length is 12000 for now, when I tried to make it more, i got a memory error.
#define ARRAY_LENGTH 9100

// address we will assign if dual sensor is present
#define LOX1_ADDRESS 0x30

// max range in millimeters
#define MAX_RANGE_VL53L0X_IN_MM 2000

// set the pins to shutdown
#define SHT_LOX1 7
#define SHT_LOX2 6

// objects for the vl53l0x
Adafruit_VL53L0X lox1 = Adafruit_VL53L0X();

// this holds the measurement
VL53L0X_RangingMeasurementData_t measure1;

// ToF sensor can measure till 200 cm on max range.
uint16_t counted_measures_each_mm[MAX_RANGE_VL53L0X_IN_MM];

/*
    Reset all sensors by setting all of their XSHUT pins low for delay(10), then set all XSHUT high to bring out of reset
    Keep sensor #1 awake by keeping XSHUT pin high
    Put all other sensors into shutdown by pulling XSHUT pins low
    Initialize sensor #1 with lox.begin(new_i2c_address) Pick any number but 0x29 and it must be under 0x7F. Going with 0x30 to 0x3F is probably OK.
    Keep sensor #1 awake, and now bring sensor #2 out of reset by setting its XSHUT pin high.
    Initialize sensor #2 with lox.begin(new_i2c_address) Pick any number but 0x29 and whatever you set the first sensor to
 */

bool is_finished, is_done_measuring, is_array_full, is_time_up = false;
int seconds_to_measure = 300;
int seconds_before_starting = 60;
int counter = 0;
uint16_t measures[ARRAY_LENGTH];
int out_of_scope_measurements = 0;
  
float calculate_SD() {
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

uint16_t calculate_median() {
  // qsort - last parameter is a function pointer to the sort function
  quickSort(measures, 0, counter - 1);
  return measures[int(counter / 2)];
}
    
uint16_t calculate_avarage() {
  int sum;
  for(int i = 0; i < counter; i++) {
    sum += measures[i]; 
  }
  return sum / counter;
}

void print_result() {
  if(is_array_full)
    Serial.println("The array went full.");
  
  if(is_time_up)
    Serial.println("Measure time is done.");
  
  Serial.print("Amount measures that were taken in range: ");
  Serial.print(counter);
  Serial.println();

  Serial.print("Amount measures that were taken out of range: ");
  Serial.print(out_of_scope_measurements);
  Serial.println();
    
  Serial.print("Avarage: ");
  Serial.print(calculate_avarage());
  Serial.println();
  
  Serial.print("Standard deviation: ");
  Serial.print(calculate_SD());
  Serial.println();

  Serial.print("Median: ");
  Serial.print(calculate_median());
  Serial.println();

  Serial.print("Shortest measure: ");
  Serial.print(measures[0]);
  Serial.println();

  Serial.print("Longest measure: ");
  Serial.print(measures[counter - 1]);
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
}

void update() {
  // check if sensor has not a timeout.
  //if (sensor.timeoutOccurred()) { Serial.print(" TIMEOUT"); }
  
  if(!is_finished) {  
    is_time_up = (millis() / 1000) > seconds_to_measure;
    bool is_starting_up_done = (millis() / 1000) > seconds_before_starting;
    is_array_full = counter >= ARRAY_LENGTH;
    lox1.rangingTest(&measure1, false); // pass in 'true' to get debug data printout!
    Serial.println();
    uint16_t measurement_in_mm = measure1.RangeMilliMeter;
        
    if(is_time_up || is_array_full) {
      is_finished = true;
      print_result();
    } else {
      if(is_starting_up_done) {
        Serial.print("Current measure in mm : ");      
        Serial.print(measurement_in_mm);
        Serial.println();
        //Serial.print("Time left : ");
        //Serial.print(seconds_to_measure - ( millis() / 1000 ));
        //Serial.println();
        if(measurement_in_mm != 0 && measurement_in_mm <= 1200) {
            measures[counter++] = measurement_in_mm;
            counted_measures_each_mm[measurement_in_mm]++;
        } else {
            out_of_scope_measurements++;  
        }
      } else {
        Serial.print("Seconds before starting the test: ");
        Serial.print(seconds_before_starting - (millis() / 1000));
        Serial.println();
        Serial.print("Measurement right now: ");
        Serial.print(measurement_in_mm);
        Serial.println();
      }
    }
  }
}

void setID() {
  // all reset
  digitalWrite(SHT_LOX1, LOW);    
  delay(10);
  // all unreset
  digitalWrite(SHT_LOX1, HIGH);
  delay(10);

  // activating LOX1 and reseting LOX2
  digitalWrite(SHT_LOX1, HIGH);

  // initing LOX1
  if(!lox1.begin(LOX1_ADDRESS)) {
    Serial.println(F("Failed to boot first VL53L0X"));
    while(1);
  }
}

void setup() {
  Serial.begin(115200);

  // wait until serial port opens for native USB devices
  while (! Serial) { delay(1); }

  pinMode(SHT_LOX1, OUTPUT);
  Serial.println("Shutdown pins inited...");

  digitalWrite(SHT_LOX1, LOW);

  Serial.println("Both in reset mode...(pins are low)");
  
  Serial.println("Starting...");
  setID();
}

void loop() {
   update(); 
//  read_dual_sensors();
}
