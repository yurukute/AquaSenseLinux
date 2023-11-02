#include "../include/amvif08.hpp"
#include <algorithm>
#include <arpa/inet.h>
#include <fcntl.h>
#include <unistd.h>

enum class Modbus {
    read        = 0x03,
    read_input  = 0x04,
    write       = 0x06,
    write_multi = 0x10,
};

enum class Register {
  auto_report = 0x00F6,
  product_id  = 0x00F7,
  factory_rst = 0x00FB,
  return_time = 0x00FC,
  rs485_addr  = 0x00FD,
  baudrate    = 0x00FE,
  parity      = 0x00FF,
};

// Since the module has 7 channels, read data can be up to 14 bytes.
// Plus 1 byte data size and 2 CRC bytes.
#define BUFFER_SIZE 17
#define ADDR_MAX 247
#define CH_MAX 8
#define BROADCAST 0xFF

#ifdef DEBUG
#include <iostream>
#include <iomanip>
#include <sstream>
#define DEBUG_PRINT(MSG)               \
{                                      \
    std::cerr << __func__ << ", line " \
              << __LINE__ << ":\t"     \
              << MSG << "\n";          \
}
#else
#define DEBUG_PRINT(MSG)
#endif

bool AMVIF08::isValid(short ch) {
    return (ch >= 1 && ch <= CH_MAX);
}

int AMVIF08::send(uint8_t rs485_addr, Modbus func, uint32_t data) {
    uint8_t request[8];
    uint8_t response[BUFFER_SIZE];
    int read_size;

    request[0] = rs485_addr;
    request[1] = static_cast<uint8_t>(func);
    *(uint32_t *)(&request[2]) = htonl(data);
    *(uint16_t *)(&request[6]) = calculateCRC(&request[0], 6);

    write(fd, request, 8);
    sleep(1);
    
    read_size = read(fd, response, sizeof(response));

#ifdef DEBUG
    std::ostringstream msg;
    msg << std::hex << std::setfill('0') << std::uppercase;
    for (int x : request) {
        msg << std::setw(2) << x << " ";
    }
    DEBUG_PRINT("Send message:\t" << msg.str());
    if (read_size > 0) {
        msg.str("");
        for (int i = 0; i < read_size; i++) {
            msg << std::setw(2)
                << static_cast<int>(response[i]) << " ";
        }
        DEBUG_PRINT("Return:      \t" << msg.str());
    }
    else DEBUG_PRINT("Send failed.");
#endif

    if (read_size > 0) {
        switch (func) {
        case Modbus::read: case Modbus::read_input:
            int data_size, header;
            if (rs485_addr == BROADCAST) {
                data_size = response[0]/2;
                header = 1;
            }
            else {
                data_size = response[2]/2;
                header = 3;
            }
            for (auto i = 0; i < data_size; i++) {
                // Skip header, read 2 bytes each.
                auto pos = 2*i+header;
                read_data[i] = response[pos] << 8 | response[pos+1];
            }
            break;
        case Modbus::write:
            {
                uint16_t *set_value = (uint16_t *)(&request[4]);
                uint16_t *reg_value = (uint16_t *)(&response[4]);
                if(*set_value != *reg_value) {
                    read_size = -1;
                }
                break;
            }
        case Modbus::write_multi:
            // TODO
            break;
        } 
    }
    return read_size;
}

int AMVIF08::connect(const char *port) {
    fd = open(port, O_RDWR | O_NOCTTY | O_NDELAY);
    if (fd < 0) {
        DEBUG_PRINT("Open port" << port << " failed: " << errno);
        return -1;
    }

    // Find device's address
    send(BROADCAST, Modbus::read,
         static_cast<uint16_t>(Register::rs485_addr) << 16 | 0x01);
    addr = read_data[0];
    if (!addr) { // Address not found.
        DEBUG_PRINT("Cannot find device address.");
        return -1;
    } else DEBUG_PRINT("Found at: " << addr);

    // Find baud rate
    send(addr, Modbus::read,
         static_cast<uint16_t>(Register::baudrate) << 16 | 0x01);
    baudrate = read_data[0];
    return fd;
}

std::vector<float>  AMVIF08::readVoltage(uint32_t ch, uint8_t number) {
    std::vector<float> voltage;
    if (isValid(ch) == false) {
        DEBUG_PRINT("Invalid channel: " << ch);
    }
    else if (isValid(ch + number -1) == false) {
        DEBUG_PRINT("Invalid read number: " << number);
    }
    // Channel 1-7 indicated at 0x00A0-0x00A7.
    else if (send(addr, Modbus::read, (ch+0xA0) << 16 | number) < 1) {
        DEBUG_PRINT("Cannot read voltage values.");
    }
    else {
        for (auto i = 0; i < number; i++) {
            voltage.push_back((float) read_data[i] / 100.0);
        }
    }
    return voltage;
}

std::vector<float> AMVIF08::getVoltageRatio(uint32_t ch,
                                            uint8_t number) {
    std::vector<float> ratio;
    if (isValid(ch) == false) {
        DEBUG_PRINT("Invalid channel: " << ch);
    }
    else if (isValid(ch + number -1) == false) {
        DEBUG_PRINT("Invalid read number: " << number);
    }
    // Channel 1-7 indicated at 0x00C0-0x00C7.
    else if (send(addr, Modbus::read, (ch+0xC0) << 16 | number) < 1) {
        DEBUG_PRINT("Cannot read voltage ratios.");
    }
    else {
        for (auto i = 0; i < number; i++) {
            ratio.push_back((float) read_data[i] / 1000.0);
        }
    }
    return ratio;
}

short AMVIF08::factoryReset() {
    return send(BROADCAST, Modbus::write,
                static_cast<uint16_t>(Register::factory_rst) << 16);
}

short AMVIF08::setReturnTime(short msec) {
    if (msec < 0 || msec > 1000) {
        DEBUG_PRINT("Invalid return time");
        return -1;
    }
    uint16_t reg = static_cast<uint16_t>(Register::return_time);
    if (send(addr, Modbus::write, reg << 16 | (msec / 40)) < 0) {
        return -1;
    }
    return_time = msec;
    return 0;
}

short AMVIF08::setAddr(short newaddr) {
    if (newaddr < 1 || newaddr > ADDR_MAX) {
        DEBUG_PRINT("Invalid address (1-" << ADDR_MAX << ").");
        return -1;
    }
    
    if (send(addr, Modbus::write,
             static_cast<int>(Register::rs485_addr) << 16 | newaddr) < 0) {
        DEBUG_PRINT("Cannot set addr.");
        return -1;
    }
    
    addr = newaddr;
    return 0;
}

short AMVIF08::setVoltageRatio(short ch, float ratio) {
    // Channel 1-8 indicated at 0x00C0-0x00C7.
    uint16_t val = ratio * 1000;
    if (send(addr, Modbus::write, (ch+0x00C0) << 16 | val) < 0) {
        DEBUG_PRINT("Cannot set voltage ratio");
        return -1;
    }
    return 0;
}

short AMVIF08::setBaudRate(short target_baud) {
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

    if (send(addr, Modbus::write,
             static_cast<uint16_t>(Register::baudrate) << 16 | baud_code) < 1) {
        DEBUG_PRINT("Cannot change baud rate.");
    }

    baudrate = target_baud;
    return 0;
}
