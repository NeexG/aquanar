#include "ph.h"
#include "config/config.h"
#include <string.h> // For memcpy

// Pre-calculated constants for performance
#define ADC_TO_VOLTAGE (3.3f / 4095.0f)  // ESP32 12-bit ADC conversion factor
#define PH_RANGE (7.0f - 4.0f)           // pH range between calibration points (3.0)
#define PH_NEUTRAL 7.0f                  // Neutral pH value
#define PH_MAX 14.0f                     // Maximum pH value
#define PH_MIN 0.0f                      // Minimum pH value

PHSensor::PHSensor(int pin) : pin(pin), ph7Voltage(2.50f), ph4Voltage(3.00f), 
                               sampleIndex(0), bufferFilled(false) {
  // Default offset will be loaded from preferences in loadCalibration()
  // If not found, default to -0.5f (maximum allowed)
  offset = -0.5f;
  calculateSlope(); // Calculate initial slope
  
  // Initialize sample buffer
  for (int i = 0; i < PH_MEDIAN_SAMPLES; i++) {
    samples[i] = 7.0f;
  }
}

void PHSensor::begin() {
  pinMode(pin, INPUT);
  loadCalibration();
  Serial.println("pH Sensor initialized");
}

void PHSensor::loadCalibration() {
  Preferences prefs;
  prefs.begin(PREF_NAMESPACE, true);
  ph7Voltage = prefs.getFloat(PREF_PH7_KEY, 2.50f);
  ph4Voltage = prefs.getFloat(PREF_PH4_KEY, 3.00f); // pH 4 typically ~3.0V
  offset = prefs.getFloat("ph_offset", -0.5f); // Load saved offset, default -0.5 (max allowed)
  
  // Validate offset: must be >= -0.5
  if (offset < -0.5f) {
    offset = -0.5f;
    Serial.println("Warning: Offset was below -0.5, reset to -0.5");
  }
  
  prefs.end();
  
  calculateSlope(); // Recalculate slope after loading calibration
  
  Serial.printf("pH calibration loaded: 7.00=%.3fV, 4.00=%.3fV, slope=%.3f pH/V, offset=%.2f\n", 
                ph7Voltage, ph4Voltage, slope, offset);
}

void PHSensor::calculateSlope() {
  // Calculate slope from calibration points
  // slope = (pH7 - pH4) / (ph4Voltage - ph7Voltage)
  // Example: (7.0 - 4.0) / (3.0 - 2.5) = 3.0 / 0.5 = 6.0 pH per volt
  float voltageDiff = ph4Voltage - ph7Voltage;
  if (voltageDiff > 0.01f || voltageDiff < -0.01f) { // Avoid division by zero
    slope = PH_RANGE / voltageDiff;
  } else {
    slope = 6.0f; // Default fallback: 3 pH units per 0.5V = 6.0 pH/V
  }
}

// Partition function for floats (must be declared before quickSelect)
static int partition(float arr[], int left, int right, int pivotIndex) {
  float pivotValue = arr[pivotIndex];
  // Swap pivot to end
  float temp = arr[pivotIndex];
  arr[pivotIndex] = arr[right];
  arr[right] = temp;
  
  int storeIndex = left;
  for (int i = left; i < right; i++) {
    if (arr[i] < pivotValue) {
      temp = arr[storeIndex];
      arr[storeIndex] = arr[i];
      arr[i] = temp;
      storeIndex++;
    }
  }
  // Move pivot to final position
  temp = arr[right];
  arr[right] = arr[storeIndex];
  arr[storeIndex] = temp;
  return storeIndex;
}

// Partition function for integers (must be declared before quickSelectInt)
static int partitionInt(int arr[], int left, int right, int pivotIndex) {
  int pivotValue = arr[pivotIndex];
  int temp = arr[pivotIndex];
  arr[pivotIndex] = arr[right];
  arr[right] = temp;
  
  int storeIndex = left;
  for (int i = left; i < right; i++) {
    if (arr[i] < pivotValue) {
      temp = arr[storeIndex];
      arr[storeIndex] = arr[i];
      arr[i] = temp;
      storeIndex++;
    }
  }
  temp = arr[right];
  arr[right] = arr[storeIndex];
  arr[storeIndex] = temp;
  return storeIndex;
}

// Quickselect algorithm to find median (O(n) average, much faster than full sort)
static float quickSelect(float arr[], int left, int right, int k) {
  if (left == right) return arr[left];
  
  int pivotIndex = left + ((right - left) >> 1); // Use middle as pivot
  pivotIndex = partition(arr, left, right, pivotIndex);
  
  if (k == pivotIndex) {
    return arr[k];
  } else if (k < pivotIndex) {
    return quickSelect(arr, left, pivotIndex - 1, k);
  } else {
    return quickSelect(arr, pivotIndex + 1, right, k);
  }
}

float PHSensor::calculateMedian() {
  // Optimized: Use quickselect (O(n) average) instead of full sort
  static float temp[PH_MEDIAN_SAMPLES];
  memcpy(temp, samples, sizeof(samples));
  
  int medianIndex = PH_MEDIAN_SAMPLES >> 1; // Divide by 2 using bit shift
  return quickSelect(temp, 0, PH_MEDIAN_SAMPLES - 1, medianIndex);
}

float PHSensor::read() {
  // Optimized: Single ADC reading with fast median filter
  int adcValue = getFastMedianADC();
  
  // Optimized: Pre-calculated constant for voltage conversion
  float voltage = adcValue * ADC_TO_VOLTAGE;
  
  // ===== pH Calculation =====
  // Main pH formula: pH = 7.0 + (ph7Voltage - voltage) * slope + offset
  // Optimized: Combine calculations
  float phValue = PH_NEUTRAL + (ph7Voltage - voltage) * slope + offset;
  
  // Optimized: Fast clamp using conditional assignment
  phValue = (phValue < PH_MIN) ? PH_MIN : ((phValue > PH_MAX) ? PH_MAX : phValue);
  
  // Add to median filter buffer for pH values
  samples[sampleIndex] = phValue;
  
  // Optimized: Use bitwise AND for modulo when size is power of 2, otherwise use modulo
  sampleIndex++;
  if (sampleIndex >= PH_MEDIAN_SAMPLES) {
    sampleIndex = 0;
    if (!bufferFilled) bufferFilled = true;
  }
  
  // Return median if buffer is filled, otherwise return current value
  return bufferFilled ? calculateMedian() : phValue;
}

// Quickselect for integers (optimized version)
static int quickSelectInt(int arr[], int left, int right, int k) {
  if (left == right) return arr[left];
  
  int pivotIndex = left + ((right - left) >> 1);
  pivotIndex = partitionInt(arr, left, right, pivotIndex);
  
  if (k == pivotIndex) {
    return arr[k];
  } else if (k < pivotIndex) {
    return quickSelectInt(arr, left, pivotIndex - 1, k);
  } else {
    return quickSelectInt(arr, pivotIndex + 1, right, k);
  }
}

int PHSensor::getFastMedianADC() {
  // Optimized: Reduced samples (9 instead of 11) for even faster response
  // Still provides excellent noise filtering
  const int samples = 9;
  static int buf[samples];
  
  // Take multiple readings with minimal delay (ADC needs time to settle)
  for (int i = 0; i < samples; i++) {
    buf[i] = analogRead(pin);
    delayMicroseconds(50); // 50µs delay (faster, ADC settles quickly)
  }
  
  // Optimized: Use quickselect (O(n) average) instead of full sort
  int medianIndex = samples >> 1; // Divide by 2 using bit shift
  return quickSelectInt(buf, 0, samples - 1, medianIndex);
}

int PHSensor::getMedianADC() {
  // Calibration method (uses more samples for accuracy)
  const int samples = 15;
  static int buf[samples];
  
  // Take multiple readings with small delay
  for (int i = 0; i < samples; i++) {
    buf[i] = analogRead(pin);
    delay(5); // 5ms delay for calibration accuracy
  }
  
  // Optimized: Use quickselect for calibration too (faster than insertion sort)
  int medianIndex = samples >> 1;
  return quickSelectInt(buf, 0, samples - 1, medianIndex);
}

void PHSensor::calibrate7() {
  // Use median filter for calibration reading (more samples for accuracy)
  int adcValue = getMedianADC();
  ph7Voltage = adcValue * ADC_TO_VOLTAGE;
  
  Preferences prefs;
  prefs.begin(PREF_NAMESPACE, false);
  prefs.putFloat(PREF_PH7_KEY, ph7Voltage);
  prefs.end();
  
  calculateSlope(); // Recalculate slope
  
  Serial.printf("pH 7.00 calibrated: %.3fV, slope=%.3f pH/V\n", ph7Voltage, slope);
}

void PHSensor::calibrate4() {
  // Use median filter for calibration reading (more samples for accuracy)
  int adcValue = getMedianADC();
  ph4Voltage = adcValue * ADC_TO_VOLTAGE;
  
  Preferences prefs;
  prefs.begin(PREF_NAMESPACE, false);
  prefs.putFloat(PREF_PH4_KEY, ph4Voltage);
  prefs.end();
  
  calculateSlope(); // Recalculate slope
  
  Serial.printf("pH 4.00 calibrated: %.3fV, slope=%.3f pH/V\n", ph4Voltage, slope);
}

void PHSensor::setCalibration(float ph7, float ph4) {
  ph7Voltage = ph7;
  ph4Voltage = ph4;
  
  Preferences prefs;
  prefs.begin(PREF_NAMESPACE, false);
  prefs.putFloat(PREF_PH7_KEY, ph7Voltage);
  prefs.putFloat(PREF_PH4_KEY, ph4Voltage);
  prefs.end();
  
  calculateSlope(); // Recalculate slope
}

void PHSensor::setOffset(float off) {
  // Validate offset: must be >= -0.5
  if (off < -0.5f) {
    offset = -0.5f;
    Serial.printf("Warning: Offset %.2f is below -0.5, clamped to -0.5\n", off);
  } else {
    offset = off;
  }
  
  // Save offset to preferences
  Preferences prefs;
  prefs.begin(PREF_NAMESPACE, false);
  prefs.putFloat("ph_offset", offset);
  prefs.end();
  
  Serial.printf("pH offset set to: %.2f\n", offset);
}

void PHSensor::adjustOffsetForNormalWater(float targetPH) {
  // Take multiple readings and average for more accuracy
  float sum = 0.0f;
  const int readings = 5;
  
  for (int i = 0; i < readings; i++) {
    sum += read();
    delay(100); // Small delay between readings
  }
  
  float currentPH = sum / readings;
  
  // Calculate the difference needed
  float difference = targetPH - currentPH;
  
  // Adjust offset to bring reading to target, but clamp to -0.5 minimum
  float newOffset = offset + difference;
  
  if (newOffset < -0.5f) {
    offset = -0.5f;
    Serial.printf("Warning: Calculated offset %.2f is below -0.5, clamped to -0.5\n", newOffset);
    Serial.printf("Note: pH reading may still be high. Consider recalibrating pH 7.00 instead.\n");
  } else {
    offset = newOffset;
  }
  
  // Save offset to preferences
  Preferences prefs;
  prefs.begin(PREF_NAMESPACE, false);
  prefs.putFloat("ph_offset", offset);
  prefs.end();
  
  Serial.printf("pH offset adjusted: Current=%.2f, Target=%.2f, Adjustment=%.2f, New Offset=%.2f\n", 
                currentPH, targetPH, difference, offset);
  
  // Verify the adjustment
  delay(200);
  float verifyPH = read();
  Serial.printf("Verification reading: %.2f (should be close to %.2f)\n", verifyPH, targetPH);
  
  if (verifyPH > 7.5f && offset == -0.5f) {
    Serial.println("Recommendation: Recalibrate pH 7.00 buffer solution for better accuracy");
  }
}

bool PHSensor::isSafe() {
  float currentPH = read();
  return (currentPH >= PH_MIN_SAFE && currentPH <= PH_MAX_SAFE);
}

