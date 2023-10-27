#include "../include/vernier.hpp"
#include <cstring>

#define RESOLUTION 4096

Vernier::~Vernier() {};

float Vernier::readSensor(float voltage) {
    return slope * voltage + intercept;
}

float Vernier::readSensor(int rawADC) {
    return slope * rawADC*Vin/RESOLUTION + intercept;
}

void Vernier::calibrate(float new_slope, float new_intercept) {
    slope = new_slope;
    intercept = new_intercept;
}

// END OF VERNIER IMPLEMENTATION

SSTempSensor::SSTempSensor() {
    slope = 0;
    intercept = 1;
    responseTime = 10;
    strcpy(sensorUnit, "Deg C");
}

void SSTempSensor::switchUnit() {
    if(strcmp(sensorUnit, "Deg C") == 0) {
        strcpy(sensorUnit, "Deg F");
    }
    else strcpy(sensorUnit, "Deg C");
}

float SSTempSensor::calculateTemp(float resistance) {
    if (resistance < 0){
        return -1;
    }
    // Dual-purpose variable to save space.
    float temp = log(resistance);
    temp = 1/(K0 + K1*temp + K2*temp*temp*temp) - 273.15;
    
    if (strcmp(sensorUnit, "Deg F") == 0){
        temp = 1.8*temp + 32.0; // Convert to Fahrenheit
    }

    return temp;
}

float SSTempSensor::readSensor(float voltage) {
    return calculateTemp(voltage * divider / (Vin - voltage));
}

float SSTempSensor::readSensor(int rawADC) {
    return calculateTemp(rawADC * divider / (RESOLUTION - rawADC));
}

// END OF TEMPSENSOR IMPLEMENTATION.

ODOSensor::ODOSensor() {
    slope = 4.444;
    intercept = -0.4444;
    responseTime = 40;
    strcpy(sensorUnit, "mg/L");
}

void ODOSensor::switchUnit(){
    if(strcmp(sensorUnit, "mg/L") == 0) {
        slope = 66.666;
        intercept = -6.6666;
        strcpy(sensorUnit, "%");
    }
    else {
        slope = 4.444;
        intercept = -0.4444;
        strcpy(sensorUnit, "mg/L");
    }
}

// END OF ODOSENSOR IMPLEMENTATION

FPHSensor::FPHSensor() {
    slope = -7.78;
    intercept = 16.34;
    responseTime = 1;
    sensorUnit[0] = '\0'; // pH don't have unit
}

// END OF FPHSENSOR IMPLEMENTATION