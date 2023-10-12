#include "../include/r4ava07.hpp"
#include "../include/vernier.hpp"
#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <algorithm>
#include <functional>
#include <thread>
#include <vector>

#define VIN_CH 0
#define TMP_CH 1
#define ODO_CH 2
#define FPH_CH 3

using namespace std::chrono_literals;

const float Rl = 5930.434783; // ADC resistance
const int sample_rate = 10;   // 10 samples per read
const int read_num    = 4;    // Number of voltage inputs

std::vector<float> voltage_avg;
R4AVA07 ADC;

// Sensor objects
SSTempSensor TMP;
ODOSensor    ODO;
FPHSensor    FPH;

float readTemp() {
    float vin  = voltage_avg[VIN_CH];
    float vout = voltage_avg[TMP_CH];
    if (vout == 0.0) {
        return NAN;
    }
    // Schematic:
    //   Rt: Thermistor
    //   R1: Divider resistor (Vernier's BTA protoboard)
    //   Rl: Load resistor (R4AVA07 ADC)
    //
    // [GND] --- [Rt] ----- | ----- [R1] -----[VCC (5v)]
    //       |              |
    //       |-- [Rl] ----- |
    //                      |
    //               [Analog input]
    // Parallel resistance:
    // Rp   = Rt*Rl/(Rt+Rl)
    // Vout = Vin*Rp/(R1+Rp)
    float R1 = TMP.getDividerResistance();
    float Rp = 1/(vin/(R1*vout) - 1/Rl - 1/R1);
    return TMP.calculateTemp(Rp);
}

float readODO() {
    float vout = voltage_avg[ODO_CH];
    if (vout == 0.0) {
        return NAN;
    }
    return ODO.readSensor(vout);
}

float readFPH() {
    float vout = voltage_avg[FPH_CH];
    if (vout == 0.0) {
        return NAN;
    }
    return FPH.readSensor(vout);
}

void readVoltage() {
    int sample_rate = 1;
    std::fill(voltage_avg.begin(), voltage_avg.end(), 0);
    for (auto i = 0; i < sample_rate; i++) {
        std::transform(voltage_avg.begin(), voltage_avg.end(),
                       ADC.readVoltage(1, read_num).begin(),
                       voltage_avg.begin(), std::plus<float>());
    }
    // Calculate average
    for (auto &v : voltage_avg) {
        v /= sample_rate;
    }
}

int main() {
    std::cout << "Connecting R4AVA07...";
    while (ADC.connect("/dev/ttymxc1") < 0) {
        std::cout << ".";
        std::this_thread::sleep_for(1s);
    }
    std::cout << "\n";
    
    while (true) {
        readVoltage();

        std::cout << "Voltage read:\n\tCH1\tCH2\tCH3\tCH4\n";
        for (auto v : voltage_avg) {
            std::cout << "\t" << std::setprecision(2) << v;
        }
        std::cout << "\n";
        
        std::cout << "Temperature: " << readTemp()
                  << "Dissolved oxygen: " << readODO()
                  << "pH: " << readFPH();
    }
}