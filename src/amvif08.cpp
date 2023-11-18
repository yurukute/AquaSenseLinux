#include "amvif08.hpp"
#include <arpa/inet.h>
#include <fcntl.h>
#include <map>
#include <stdexcept>

#define PARITY_N 'N'
#define PARITY_O 'O'
#define PARITY_E 'E'
#define BROADCAST 0xFF
#define ADDR_MAX 247
#define CH_MAX 8

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
  auto_report = 0x00F6,
  product_id  = 0x00F7,
  factory_rst = 0x00FB,
  return_time = 0x00FC,
  rs485_addr  = 0x00FD,
  baudrate    = 0x00FE,
  parity      = 0x00FF,
};

struct Defaults {
    static const unsigned short prod_id     = 2048;
    static const unsigned short return_time = 1000;
    static const unsigned short baudrate    = 9600;
    static const char  parity               = PARITY_N;
};

std::map<short, uint16_t> baudrates {
    {1200, 0},
    {2400, 1},
    {4800, 2},
    {9600, 3},
    {19200, 4},
    {38400, 5},
    {57600, 6},
    {115200, 7}
};

bool AMVIF08::isValidChannel(unsigned short ch) {
    return (ch > 0 && ch <= CH_MAX);
}

int AMVIF08::connect(const char *port) {
    ctx = modbus_new_rtu(port, Defaults::baudrate, Defaults::parity, 8, 1);
    if (ctx == NULL) {
        DEBUG_PRINT("Failed to create context: "
                    << modbus_strerror(errno));
        return -1;
    }

#ifdef DEBUG
    modbus_set_debug(ctx, true);
#endif

    // int flags =  MODBUS_QUIRK_MAX_SLAVE | MODBUS_QUIRK_REPLY_TO_BROADCAST;
    // modbus_enable_quirks(ctx, flags);
    modbus_set_slave(ctx, 1);

    if (modbus_connect(ctx) < 0) {
        DEBUG_PRINT("Cannot connect to device: "
                    << modbus_strerror(errno));
        modbus_free(ctx);
        return -1;
    }

    rs485_port = port;
    baudrate = Defaults::baudrate;
    parity = Defaults::parity;
    return_time = Defaults::return_time;
    return modbus_get_socket(ctx);
}

std::vector<float>  AMVIF08::readVoltage(uint16_t ch, uint8_t number) {
    std::vector<float> voltage;
    uint16_t read_data[number];

    if (isValidChannel(ch) == false) {
        DEBUG_PRINT("Invalid channel: " << ch);
    }
    else if (isValidChannel(ch + number -1) == false) {
        DEBUG_PRINT("Invalid read number: " << number);
    }
    // Channel 1-7 indicated at 0x00A0-0x00A7.
    else if (modbus_read_registers(ctx, ch-1+0xA0, number, read_data) < 0) {
        DEBUG_PRINT("Cannot read voltage values.");
    }
    else {
        for (int i = 0; i < number; i++) {
            voltage.push_back((float) read_data[i] / 100.0);
        }
    }
    return voltage;
}

std::vector<float> AMVIF08::getVoltageRatio(uint16_t ch, uint8_t number) {
    std::vector<float> ratio;
    uint16_t read_data[number];

    if (isValidChannel(ch) == false) {
        DEBUG_PRINT("Invalid channel: " << ch);
    }
    else if (isValidChannel(ch + number -1) == false) {
        DEBUG_PRINT("Invalid read number: " << number);
    }
    // Channel 1-7 indicated at 0x00C0-0x00C7.
    else if (modbus_read_registers(ctx, ch-1+0xC0, number, read_data) < 0) {
        DEBUG_PRINT("Cannot read voltage ratios.");
    }
    else {
        for (int i = 0; i < number; i++) {
            ratio.push_back((float) read_data[i] / 1000.0);
        }
    }
    return ratio;
}

short AMVIF08::factoryReset() {
    modbus_set_slave(ctx, BROADCAST);

    if (modbus_write_register(ctx, static_cast<uint16_t>(Registers::factory_rst), 0) < 0) {
        return -1;
    }

    modbus_set_slave(ctx, 1);
    return_time = Defaults::return_time;
    baudrate    = Defaults::baudrate;
    parity      = Defaults::parity;
    return 0;
}

short AMVIF08::setReturnTime(uint16_t msec) {
    if (msec > 1000) {
        DEBUG_PRINT("Invalid return time.");
        return -1;
    }

    if (modbus_write_register(ctx, static_cast<uint16_t>(Registers::return_time), msec / 40) < 0) {
        DEBUG_PRINT("Cannot set return time.");
        return -1;
    }

    return_time = msec;
    return 0;
}

short AMVIF08::setAddr(unsigned short newaddr) {
    if (newaddr < 1 || newaddr > ADDR_MAX) {
        DEBUG_PRINT("Invalid address (1-" << ADDR_MAX << ").");
        return -1;
    }
    if (modbus_write_register(ctx, static_cast<uint16_t>(Registers::rs485_addr), newaddr) < 0) {
        DEBUG_PRINT("Cannot set address.");
        return -1;
    }
    
    modbus_set_slave(ctx, newaddr);
    return 0;
}

short AMVIF08::setVoltageRatio(unsigned short ch, float ratio) {
    if (isValidChannel(ch) == false) {
        DEBUG_PRINT("Invalid channel: " << ch);
        return -1;
    }
    if (ratio < 0 || ratio > 1) {
        DEBUG_PRINT("Invalid ratio value: " << ratio);
        return -1;
    }
    // Channel 1-8 indicated at 0x00C0-0x00C7.
    if (modbus_write_register(ctx, ch-1 + 0x00C0, ratio * 1000) < 0) {
        DEBUG_PRINT("Cannot set voltage ratio.");
        return -1;
    }

    return 0;
}

short AMVIF08::setBaudRate(unsigned short target_baud) {
    uint16_t baud_code;
    try {
        baud_code = baudrates.at(target_baud);
    }
    catch (const std::out_of_range &e) {
        DEBUG_PRINT("Not supported baudrate.");
        return -1;
    }

    if (modbus_write_register(ctx, static_cast<uint16_t>(Registers::baudrate), baud_code) < 0) {
        DEBUG_PRINT("Cannot change baudrate: "
                    << modbus_strerror(errno));
        return -1;
    }

    baudrate = target_baud;
    updateContext();
    return 0;
}

short AMVIF08::setParity(char type) {
    uint16_t paritycode = Defaults::parity;
    switch (type) {
    case PARITY_N: paritycode = 0; break;
    case PARITY_O: paritycode = 1; break;
    case PARITY_E: paritycode = 2; break;
    default:
        DEBUG_PRINT("Invalid parity type.");
    }

    if(modbus_write_register(ctx, static_cast<uint16_t>(Registers::rs485_addr), paritycode) < 0) {
        DEBUG_PRINT("Cannot set parity: " << modbus_strerror(errno));
        return -1;
    }

    parity = paritycode;
    updateContext();
    return 0;
}

void AMVIF08::updateContext() {
    modbus_free(ctx);
    ctx = modbus_new_rtu(rs485_port.c_str(), baudrate, parity, 8, 1);
}