/* This example shows how to use continuous mode to take
range measurements with the VL53L0X. It is based on
vl53l0x_ContinuousRanging_Example.c from the VL53L0X API.

The range readings are in units of mm. */
#define TOF_1_XSHUT_PIN   6
#define TOF_1_GPIO_PIN    5  
#define TOF_2_XSHUT_PIN   10
#define TOF_2_GPIO_PIN    9
#define TOF_1_ADDR        0x30
#define TOF_2_ADDR        0x31

#include <Wire.h>
#include <VL53L0X.h>
// array length is 12000 for now, when I tried to make it more, i got a memory error.
#define ARRAY_LENGTH 10000
#define MAX_RANGE_VL53L0X_IN_MM 2000

VL53L0X sensor;
bool is_finished, is_done_measuring, is_array_full, is_time_up = false;
int seconds_for_starting_up = 60;
int seconds_to_measure = 300 + seconds_for_starting_up;
int counter = 0;
uint16_t measures[ARRAY_LENGTH];
// ToF sensor can measure till 200 cm on max range.
uint16_t counted_measures_each_mm[MAX_RANGE_VL53L0X_IN_MM];
int incorrect_measures = 0;

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
    
int calculate_avarage() {
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
  
  Serial.print("Amount measures that were taken : ");
  Serial.print(counter);
  Serial.println();

  Serial.print("Measures that missed the target: ");
  Serial.println(incorrect_measures);
  
  Serial.print("Avarage mm: ");
  Serial.print(calculate_avarage());
  Serial.println();
  
  Serial.print("Standard deviation mm: ");
  Serial.print(calculate_SD());
  Serial.println();

  Serial.print("Median mm: ");
  Serial.print(calculate_median());
  Serial.println();

  Serial.print("Shortest measure in mm: ");
  Serial.print(measures[0]);
  Serial.println();

  Serial.print("Longest measure in mm: ");
  Serial.print(measures[counter - 1]);
  Serial.println();

  Serial.println("Measurements taken from sensor: ");
  
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
  // Check if startup time is over.
  bool is_startup_done = (millis() / 1000) > seconds_for_starting_up;
  
  if(is_startup_done && !is_finished) {  
    is_time_up = (millis() / 1000) > seconds_to_measure;
    is_array_full = counter >= ARRAY_LENGTH;
    //store a sensor measurement.
    uint16_t measure = sensor.readRangeContinuousMillimeters();
    bool timeOutOccurred = false;
    
    if(is_time_up || is_array_full) {
      is_finished = true;
      print_result();
    } else {
      if(measure > 0 && measure < 1200 && !timeOutOccurred) {

        measures[counter++] = measure;
        counted_measures_each_mm[measure]++;
        Serial.println(measure);
        Serial.print(" Time left for measuring : ");
        Serial.print(seconds_to_measure - millis() / 1000);
        Serial.println();  
      } else {
        Serial.println("0");
        incorrect_measures++;
      }
    } 
  } else {
      if(!is_startup_done) {
        Serial.print("Seconds before starting: ");
        Serial.println(seconds_for_starting_up - (millis() / 1000));
        Serial.print("Range measurement of sensor 1 in mm: ");
        int test = millis();
        Serial.println(sensor.readRangeContinuousMillimeters());
        Serial.println(millis() - test);
        Serial.println();  
      } 
    }
}

void setup()
{
  Serial.begin(9600);
  Wire.begin();

  sensor.setTimeout(500);

  //TOF sensor
  pinMode(TOF_1_XSHUT_PIN, OUTPUT); 
  pinMode(TOF_2_XSHUT_PIN, OUTPUT); 
  pinMode(TOF_1_GPIO_PIN, INPUT); 
  pinMode(TOF_2_GPIO_PIN, INPUT); 
  digitalWrite(TOF_1_XSHUT_PIN, 1); 
  digitalWrite(TOF_2_XSHUT_PIN, 1);
  
  if (!sensor.init())
  {
    Serial.println("Failed to detect and initialize sensor!");
    while (1) {}
  }

  // Start continuous back-to-back mode (take readings as
  // fast as possible).  To use continuous timed mode
  // instead, provide a desired inter-measurement period in
  // ms (e.g. sensor.startContinuous(100)).
  sensor.startContinuous();
}

void loop()
{
  update();
}
