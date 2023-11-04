#include "amvif08.hpp"
#include "vernier.hpp"
#include <algorithm>
#include <functional>
#include <iomanip>
#include <iostream>
#include <thread>
#include <vector>

#define VIN_CH 0
#define TMP_CH 1
#define ODO_CH 2
#define FPH_CH 3

#ifndef PORT
#define PORT "/dev/ttymxc1"
#endif

const float Rl = 5930.434783;   // ADC resistance
const int read_num = 4;         // Number of voltage inputs
const int sample_rate = 10;     // 10 samples per read

std::vector<float> voltage_sum(read_num); // Store sum of voltage;
float tmp = NAN, odo = NAN, fph = NAN;
AMVIF08 ADC;

void readTemp() {
    SSTempSensor TMP;
    std::chrono::seconds response_time((int) TMP.getResponseTime());
    while (true) {
        std::this_thread::sleep_for(response_time);
        // Calculate average
        float vin  = voltage_sum[VIN_CH] / response_time.count();
        float vout = voltage_sum[TMP_CH] / response_time.count();
        // Reset readings
        voltage_sum[VIN_CH] = 0;
        voltage_sum[TMP_CH] = 0;
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
        if (vout > 0.0) {
            float R1 = TMP.getDividerResistance();
            float Rp = 1/(vin/(R1*vout) - 1/Rl - 1/R1);
            tmp = TMP.calculateTemp(Rp);
        }
        else tmp = NAN;
    }
}

void readODO() {
    ODOSensor ODO;
    std::chrono::seconds response_time((int) ODO.getResponseTime());
    while (true) {
        std::this_thread::sleep_for(response_time);
        // Calculate average
        float vout = voltage_sum[ODO_CH] / response_time.count();
        // Reset reading
        voltage_sum[ODO_CH] = 0;
        if (vout > 0.0) {
            odo = ODO.readSensor(vout);
        }
        else vout = NAN;
    }
}

void readFPH() {
    FPHSensor FPH;
    std::chrono::seconds response_time((int) FPH.getResponseTime());
    while (true) {
        std::this_thread::sleep_for(response_time);
        float vout = voltage_sum[FPH_CH] / response_time.count();
        if (vout > 0.0) {
            fph = FPH.readSensor(vout);
        }
        else fph = NAN;
    }
}

void readVoltage() {
    std::vector<float> voltage_read;
    voltage_read = ADC.readVoltage(1, read_num);
    std::transform(voltage_sum.begin(), voltage_sum.end(),
                   voltage_read.begin(),
                   voltage_sum.begin(), std::plus<float>());

    std::cout << "Voltage read:\n\tCH1\tCH2\tCH3\tCH4\n"
              << std::setprecision(2);
    for (auto v : voltage_read) {
        std::cout << "\t" << v;
    }
    std::cout << "\n";
}

int main() {
    using namespace std::chrono_literals;

    std::cout << "Connecting ..." << std::flush;
    while (ADC.connect(PORT) < 0) {
        std::cout << "." << std::flush;
        std::this_thread::sleep_for(1s);
    }
    std::cout << std::endl;

    std::thread temp_reader(readTemp);
    temp_reader.detach();

    std::thread odo_reader(readODO);
    odo_reader.detach();

    std::thread fph_reader(readFPH);
    fph_reader.detach();

    while (true) {
        std::chrono::milliseconds delay_time(1000/sample_rate);
        for (int i = 0; i < sample_rate; i++) {
            std::thread voltage_reader(readVoltage);
            voltage_reader.detach();
            std::this_thread::sleep_for(delay_time);
        }
        std::this_thread::sleep_for(2s);
        std::cout << "Temperature: " << tmp
                  << "\nDissolved oxygen: " << odo
                  << "\npH: " << fph
                  << std::endl;
    }
}