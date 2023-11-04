#include "r4ava07.hpp"
#include <algorithm>
#include <arpa/inet.h>
#include <fcntl.h>
#include <map>
#include <stdexcept>
#include <unistd.h>

#define BROADCAST 0xFF
#define ID_MAX 247
#define CH_MAX 7

enum class Registers : uint16_t {
  rs485_addr  = 0x000E,
  baudrate    = 0x000F,
};

unsigned short read_data[7] = {0x00};
std::map<short, uint16_t> baudrate {
    {1200, 0},
    {2400, 1},
    {4800, 2},
    {9600, 3},
    {19200, 4}
};

bool R4AVA07::isValid(short ch) {
    return (ch >= 1 && ch <= CH_MAX);
}

int R4AVA07::connect(const char *port) {
    int fd = ModbusDevice::connect(port);
    if (fd < 1) {
        return -1;
    }

    uint16_t read_data[1];
    // Find device's address
    sendRead(BROADCAST, static_cast<uint16_t>(Registers::rs485_addr), 0x01, read_data);
    ID = read_data[0];
    if (!ID) { // Address not found.
        DEBUG_PRINT("Cannot find device address.");
        return -1;
    }

    // Find baud rate
    sendRead(ID, static_cast<uint16_t>(Registers::baudrate), 0x01, read_data);
    baud = read_data[0];
    return fd;
}

std::vector<float>  R4AVA07::readVoltage(uint16_t ch, uint8_t number) {
    if (isValid(ch) == false) {
        DEBUG_PRINT("Invalid channel: " << ch);
        return {-1};
    }
    if (isValid(ch + number -1) == false) {
        DEBUG_PRINT("Invalid read number: " << number);
        return {-1};
    }
    // Channel 1-7 indicated at 0x0000-0x0006.
    if (sendRead(ID, (ch-1) << 16, number, read_data) < 1) {
        DEBUG_PRINT("Cannot read voltage values.");
        return {-1};
    }
    std::vector<float> voltage;
    for (auto i = 0; i < number; i++) {
        voltage.push_back((float) read_data[i] / 100.0);
    }
    return voltage;
}

std::vector<float> R4AVA07::getVoltageRatio(uint16_t ch,
                                            uint8_t number) {
    if (isValid(ch) == false) {
        DEBUG_PRINT("Invalid channel: " << ch);
        return {-1};
    }
    if (isValid(ch + number -1) == false) {
        DEBUG_PRINT("Invalid read number: " << number);
        return {-1};
    }
    // Channel 1-7 indicated at 0x0007-0x000D.
    if (sendRead(ID, (ch+6) << 16, number, read_data) < 1) {
        DEBUG_PRINT("Cannot read voltage ratios.");
        return {-1};
    }
    std::vector<float> ratio;
    for (auto i = 0; i < number; i++) {
        ratio.push_back((float) read_data[i] / 1000.0);
    }
    return ratio;
}

int R4AVA07::setID(short newID) {
    if (newID < 1 || newID > ID_MAX) {
        DEBUG_PRINT("Invalid ID (1-" << ID_MAX << ").");
        return -1;
    }
    
    if (sendWrite(ID, static_cast<uint16_t>(Registers::rs485_addr), newID) < 0) {
        DEBUG_PRINT("Cannot set ID.");
        return -1;
    }
    
    ID = newID;
    return 0;
}

int R4AVA07::setVoltageRatio(short ch, float ratio) {
    // Channel 1-7 indicated at 0x0007-0x000D.
    uint16_t val = ratio * 1000;
    if (sendWrite(ID, (ch+6), val) < 0) {
        DEBUG_PRINT("Cannot set voltage ratio");
        return -1;
    }
    return 0;
}

int R4AVA07::setBaudRate(uint16_t target_baud) {
    uint16_t baud_code;
    try {
        baud_code = baudrate.at(target_baud);
    }
    catch (const std::out_of_range &e) {
        DEBUG_PRINT("Not supported baud rate");
        return -1;
    }

    if (sendWrite(ID, static_cast<uint16_t>(Registers::baudrate), baud_code) < 1) {
        DEBUG_PRINT("Cannot change baud rate.");
    }

    baud = target_baud;
    return 0;
}

void R4AVA07::resetBaud() {
    sendWrite(ID, static_cast<uint16_t>(Registers::baudrate), 0x05);
}