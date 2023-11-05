#include "amvif08.hpp"
#include <arpa/inet.h>
#include <fcntl.h>
#include <unistd.h>

#define PARITY_N 'N'
#define PARITY_O '1'
#define PARITY_E '2'
#define ADDR_MAX 247
#define CH_MAX 8
#define BROADCAST 0xFF

struct Registers {
    static const uint16_t auto_report = 0x00F6;
    static const uint16_t product_id  = 0x00F7;
    static const uint16_t factory_rst = 0x00FB;
    static const uint16_t return_time = 0x00FC;
    static const uint16_t rs485_addr  = 0x00FD;
    static const uint16_t baudrate    = 0x00FE;
    static const uint16_t parity      = 0x00FF;
};

struct Defaults {
    static const unsigned short prod_id     = 2048;
    static const unsigned short return_time = 1000;
    static const unsigned short baudrate    = 9600;
    static const char  parity               = PARITY_N;
};

bool AMVIF08::isValidChannel(unsigned short ch) {
    return (ch > 0 && ch <= CH_MAX);
}

int AMVIF08::connect(const char *port) {
    if (ModbusDevice::connect(port) < 1) {
        return -1;
    }

    uint16_t read_data[1];
    // Find device's address
    sendRead(BROADCAST, Registers::rs485_addr, 0x01, read_data);
    addr = read_data[0];
    if (!addr) { // Address not found.
        DEBUG_PRINT("Cannot find device address.");
        return -1;
    }

    // Find baud rate
    sendRead(addr, Registers::baudrate, 0x01, read_data);
    baudrate = read_data[0];

    // Find return time
    sendRead(addr, Registers::return_time, 0x01, read_data);
    return_time = read_data[0];

    // Find parity type
    sendRead(addr, Registers::parity, 0x01, read_data);
    switch (read_data[0]) {
    case 0: parity = 'N'; break;
    case 1: parity = '1'; break;
    case 2: parity = '2'; break;
    }
    
    DEBUG_PRINT("Device found at " << (int) addr << "\n"
                "Baudrate:\t" << baudrate << "\n"
                "Return time:\t" << return_time << "\n"
                "Parity type:\t" << parity)
    return 0;
}

std::vector<float>  AMVIF08::readVoltage(uint32_t ch, uint8_t number) {
    std::vector<float> voltage;
    uint16_t read_data[number];

    if (isValidChannel(ch) == false) {
        DEBUG_PRINT("Invalid channel: " << ch);
    }
    else if (isValidChannel(ch + number -1) == false) {
        DEBUG_PRINT("Invalid read number: " << number);
    }
    // Channel 1-7 indicated at 0x00A0-0x00A7.
    else if (sendRead(addr, ch-1 + 0x00A0, number, read_data) < 0) {
        DEBUG_PRINT("Cannot read voltage values.");
    }
    else {
        for (int i = 0; i < number; i++) {
            voltage.push_back((float) read_data[i] / 100.0);
        }
    }
    return voltage;
}

std::vector<float> AMVIF08::getVoltageRatio(uint32_t ch, uint8_t number) {
    std::vector<float> ratio;
    uint16_t read_data[number];

    if (isValidChannel(ch) == false) {
        DEBUG_PRINT("Invalid channel: " << ch);
    }
    else if (isValidChannel(ch + number -1) == false) {
        DEBUG_PRINT("Invalid read number: " << number);
    }
    // Channel 1-7 indicated at 0x00C0-0x00C7.
    else if (sendRead(addr, ch-1 + 0x00C0, number, read_data)) {
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
    if (sendWrite(BROADCAST, Registers::factory_rst, 0) < 0) {
        return -1;
    }

    prod_id     = Defaults::prod_id;
    return_time = Defaults::return_time;
    baudrate    = Defaults::baudrate;
    parity      = Defaults::parity;
    return 0;
}

short AMVIF08::setReturnTime(unsigned short msec) {
    if (msec > 1000) {
        DEBUG_PRINT("Invalid return time.");
        return -1;
    }
    if (sendWrite(addr, Registers::return_time, msec / 40) < 0) {
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
    if (sendWrite(addr, Registers::rs485_addr, newaddr) < 0) {
        DEBUG_PRINT("Cannot set address.");
        return -1;
    }
    
    addr = newaddr;
    return 0;
}

short AMVIF08::setVoltageRatio(unsigned short ch, float ratio) {
    if (isValidChannel(ch) == false) {
        DEBUG_PRINT("Invalid channel: " << ch);
        return -1;
    }
    if (ratio < 0 || ratio > 1) {
        DEBUG_PRINT("Invalid ratio value: " << value);
        return -1;
    }
    // Channel 1-8 indicated at 0x00C0-0x00C7.
    if (sendWrite(addr, ch-1 + 0x00C0, ratio * 1000) < 0) {
        DEBUG_PRINT("Cannot set voltage ratio.");
        return -1;
    }
    return 0;
}

short AMVIF08::setBaudRate(unsigned short target_baud) {
    uint16_t baud_code;
    switch (target_baud) {
    case 1200:   baud_code = 0; break;
    case 2400:   baud_code = 1; break;
    case 4800:   baud_code = 2; break;
    case 9600:   baud_code = 3; break;
    case 19200:  baud_code = 4; break;
    case 38400:  baud_code = 5; break;
    case 57600:  baud_code = 6; break;
    case 115200: baud_code = 7; break;
    default:
        DEBUG_PRINT("Not supported baud rate");
        return -1;
    }

    if (sendWrite(addr, Registers::baudrate, baud_code) < 0) {
        DEBUG_PRINT("Cannot change baud rate.");
        return -1;
    }

    baudrate = target_baud;
    return 0;
}
