#include "../include/amvif08.hpp"
#include "../include/vernier.hpp"
#include <iomanip>
#include <iostream>
#include <algorithm>
#include <functional>
#include <pthread.h>
#include <unistd.h>
#include <vector>

#define VIN_CH 0
#define TMP_CH 1
#define ODO_CH 2
#define FPH_CH 3

#ifndef PORT
#define PORT "/dev/ttyS1"
#endif

const float Rl = 5930.434783; // ADC resistance
const int sample_rate = 10;   // 10 samples per read
const int read_num    = 4;    // Number of voltage inputs

std::vector<float> voltage_sum(read_num); // Store sum of voltage;
float tmp = NAN, odo = NAN, fph = NAN;
AMVIF08 ADC;

void *readTemp(void *arg) {
    SSTempSensor TMP;
    while (true) {
        sleep(TMP.getResponseTime());
        float vin  = voltage_sum[VIN_CH] / sample_rate;
        float vout = voltage_sum[TMP_CH] / sample_rate;
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
        if (vout > 0) {
            float R1 = TMP.getDividerResistance();
            float Rp = 1/(vin/(R1*vout) - 1/Rl - 1/R1);
            tmp = TMP.calculateTemp(Rp);
        }
        else tmp = NAN;
    }
}

void *readODO(void *arg) {
    ODOSensor ODO;
    while (true) {
        sleep(ODO.getResponseTime());
        float vout = voltage_sum[ODO_CH];
        if (vout > 0) {
            odo = ODO.readSensor(vout);
        }
        else odo = NAN;
    }
}

void *readFPH(void *arg) {
    FPHSensor FPH;
    while (true) {
        sleep(FPH.getResponseTime());
        float vout = voltage_sum[FPH_CH];
        if (vout > 0) {
            fph = FPH.readSensor(vout);
        }
        else fph = NAN;
    }
}

void *readVoltage(void *arg) {
    std::vector<float> voltage_read;
    voltage_read = ADC.readVoltage(1, read_num);
    if (!voltage_read.empty()){
        std::transform(voltage_sum.begin(),voltage_sum.end(),
                       voltage_read.begin(),
                       voltage_sum.begin(), std::plus<float>());

        std::cout << "Voltage read:\n\tCH1\tCH2\tCH3\tCH4\n";
        std::cout << std::setprecision(2);
        for (int i = 0; i < (int) voltage_read.size(); i++) {
            std::cout << "\t" << voltage_read[i];
        }
        std::cout << std::endl;
    }
    pthread_exit(NULL);
}

int main() {
    std::cout << "Connecting R4AVA07..." << std::flush;
    while (ADC.connect(PORT) < 0) {
        std::cout << "." << std::flush;
        sleep(1);
    }
    std::cout << std::endl;

    pthread_t tmp_reader, odo_reader, fph_reader;
    pthread_create(&tmp_reader, NULL, readTemp, NULL);
    pthread_detach(tmp_reader);

    pthread_create(&odo_reader, NULL, readODO, NULL);
    pthread_detach(odo_reader);

    pthread_create(&fph_reader, NULL, readFPH, NULL);
    pthread_detach(fph_reader);

    while (true) {
        for (int i = 0; i < sample_rate; i++) {
            pthread_t volt_reader;
            pthread_create(&volt_reader, NULL, readVoltage, NULL);
            pthread_detach(volt_reader);
            usleep(1000 / sample_rate);
        }
        sleep(1);
        std::cout << "Temperature: " << tmp
                  << "\nDissolved oxygen: " << odo
                  << "\npH: " << fph
                  << std::endl;
    }
}