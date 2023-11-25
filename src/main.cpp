#include "amvif08.hpp"
#include "vernier.hpp"
#include <algorithm>
#include <chrono>
#include <cmath>
#include <functional>
#include <iomanip>
#include <iostream>
#include <string>
#include <thread>
#include <vector>
#include <mosquittopp.h>

#define VIN_CH 0
#define TMP_CH 1
#define ODO_CH 2
#define FPH_CH 3
#define BOARD "matrix752"

#ifndef PORT
#define PORT "/dev/ttymxc1"
#endif

#ifndef SERVER
#define SERVER "test.mosquitto.org"
#define TCP_PORT 1883
#endif

using namespace std::chrono;

const int read_num = 4;         // Number of voltage inputs
const int sample_rate = 10;     // 10 samples per read

const char *tmp_topic = BOARD "/vernier/tmp-bta";
const char *odo_topic = BOARD "/vernier/odo-bta";
const char *fph_topic = BOARD "/vernier/fph-bta";

std::vector<float> voltage_avg(read_num); // Store sum of voltage;
float tmp = NAN, odo = NAN, fph = NAN;
AMVIF08 ADC;
mosqpp::mosquittopp matrix752;

void readTemp() {
    SSTempSensor TMP;
    int response_time = TMP.getResponseTime();
    std::this_thread::sleep_for(seconds(response_time));

    while (true) {
        float vout = voltage_avg[TMP_CH];
        // Schematic:
        //   Rt: Thermistor
        //   R1: Divider resistor (Vernier's BTA protoboard)
        //
        // [GND] --- [Rt] ----- | ----- [R1] -----[VCC (5v)]
        //                      |
        //               [Analog input]
        if (vout > 0.0) {
            tmp = TMP.readSensor(vout);
        }
        else tmp = NAN;
        std::this_thread::sleep_for(1s);
    }
}

void readODO() {
    ODOSensor ODO;
    int response_time = ODO.getResponseTime();
    std::this_thread::sleep_for(seconds(response_time));

    while (true) {
        // Calculate average
        float vout = voltage_avg[ODO_CH];
        if (vout > 0.0) {
            odo = ODO.readSensor(vout);
        }
        else vout = NAN;
        std::this_thread::sleep_for(1s);
    }
}

void readFPH() {
    FPHSensor FPH;
    int response_time = FPH.getResponseTime();
    std::this_thread::sleep_for(seconds(response_time));

    while (true) {
        float vout = voltage_avg[FPH_CH];
        if (vout > 0.0) {
            fph = FPH.readSensor(vout);
        }
        else fph = NAN;
        std::this_thread::sleep_for(1s);
    }
}

void readVoltage() {
    std::vector<float> voltage_read;
    voltage_read = ADC.readVoltage(1, read_num);
    if (voltage_read.empty()) {
        return;
    }
    std::transform(voltage_avg.begin(), voltage_avg.end(),
                   voltage_read.begin(),
                   voltage_avg.begin(), std::plus<float>());

    std::cout << "Voltage read:\n\tCH1\tCH2\tCH3\tCH4\n"
              << std::setprecision(2);
    for (auto v : voltage_read) {
        std::cout << "\t" << v;
    }
    std::cout << "\n";
}

void publishSensorData(const char* topic, std::string name, float value) {
    if (std::isnan(value)) {
        value = 0.0;
    }
    
    std::string msg = "{";
    msg += "\"name\":" + name + ",";
    msg += "\"value\":" + std::to_string(value);
    msg += "}";

    matrix752.publish(NULL, topic, msg.size(), msg.c_str(), 1);
    std::cout << msg << "\n";        
}

int main() {
    std::cout << "Connecting to voltage collector..." << std::flush;
    while (ADC.connect(PORT) < 0) {
        std::cout << "." << std::flush;
        std::this_thread::sleep_for(1s);
    }
    std::cout << std::endl;

    std::cout << "Connecting to server..." << std::flush;
    while (matrix752.connect_async(SERVER, TCP_PORT) != MOSQ_ERR_SUCCESS) {
        std::cout << "." << std::flush;
        std::this_thread::sleep_for(1s);
    }
    matrix752.loop_start();

    std::thread temp_reader(readTemp);
    temp_reader.detach();

    std::thread odo_reader(readODO);
    odo_reader.detach();

    std::thread fph_reader(readFPH);
    fph_reader.detach();

    while (true) {
        for (int i = 0; i < sample_rate; i++) {
            readVoltage();
            std::this_thread::sleep_for(milliseconds(1000/sample_rate));
        }

        // Calculate averages
        for (auto &v : voltage_avg) {
            v /= sample_rate;
        }

        // Publish sensor data
        publishSensorData(tmp_topic, "Temperature", tmp);
        publishSensorData(odo_topic, "Dissolved oxygen", odo);
        publishSensorData(fph_topic, "pH", fph);
    }

    matrix752.loop_stop();
    mosqpp::lib_cleanup();
}