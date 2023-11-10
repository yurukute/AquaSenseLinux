#include "r4ava07.hpp"
#include <arpa/inet.h>
#include <fcntl.h>
#include <map>
#include <stdexcept>

#define BROADCAST 0xFF
#define ID_MAX 247
#define CH_MAX 7

#ifdef DEBUG
#include <cerrno>
#include <iostream>

#define DEBUG_PRINT(MSG)               \
{                                      \
    std::cerr << __func__ << ", line " \
              << __LINE__ << ":\t"     \
              << MSG << "\n";          \
}
#else
#define DEBUG_PRINT(MSG)
#endif

enum class Registers : uint16_t {
  rs485_addr  = 0x000E,
  baudrate    = 0x000F,
};

unsigned short read_data[7] = {0x00};
std::map<short, uint16_t> baudrates {
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
    ctx = modbus_new_rtu(port, 9600, 'N', 8, 1);
    if (ctx == NULL) {
        DEBUG_PRINT("Failed to create context: "
                    << modbus_strerror(errno));
        return -1;
    }

#ifndef DEBUG
    modbus_set_debug(ctx, 1);
#endif

    int flags =  MODBUS_QUIRK_MAX_SLAVE | MODBUS_QUIRK_REPLY_TO_BROADCAST;
    modbus_enable_quirks(ctx, flags);
    modbus_set_slave(ctx, 1);

    if (modbus_connect(ctx) < 0) {
        DEBUG_PRINT("Cannot connect to device: "
                    << modbus_strerror(errno));
        modbus_free(ctx);
        return -1;
    }

    rs485_port = port;
    baud = 9600;
    return modbus_get_socket(ctx);
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
    if (modbus_read_registers(ctx, (ch-1) << 16, number, read_data) < 1) {
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
    if (modbus_read_registers(ctx, ch + 6, number, read_data) < 1) {
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
    
    if (modbus_write_register(ctx, static_cast<uint16_t>(Registers::rs485_addr), newID) < 0) {
        DEBUG_PRINT("Cannot set ID.");
        return -1;
    }
    
    modbus_set_slave(ctx, newID);
    return 0;
}

int R4AVA07::setVoltageRatio(short ch, float ratio) {
    // Channel 1-7 indicated at 0x0007-0x000D.
    uint16_t val = ratio * 1000;
    if (modbus_write_register(ctx, ch+6, val) < 0) {
        DEBUG_PRINT("Cannot set voltage ratio");
        return -1;
    }
    return 0;
}

int R4AVA07::setBaudRate(uint16_t target_baud) {
    uint16_t baud_code;
    try {
        baud_code = baudrates.at(target_baud);
    }
    catch (const std::out_of_range &e) {
        DEBUG_PRINT("Not supported baud rate");
        return -1;
    }

    if (modbus_write_register(ctx, static_cast<uint16_t>(Registers::baudrate), baud_code) < 1) {
        DEBUG_PRINT("Cannot change baud rate.");
    }

    modbus_free(ctx);
    ctx = modbus_new_rtu(rs485_port.c_str(), target_baud, 'N', 8, 1);
    baud = target_baud;
    return 0;
}

void R4AVA07::resetBaud() {
    modbus_write_register(ctx, static_cast<uint16_t>(Registers::baudrate), 0x05);
}
