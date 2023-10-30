#include "../include/r4ava07.hpp"
#include <algorithm>
#include <arpa/inet.h>
#include <fcntl.h>
#include <thread>
#include <unistd.h>

#define MODBUS_READ  0x03
#define MODBUS_WRITE 0x06
#define BROADCAST    0xFF
#define ADDR_REG   0x000E
#define BAUD_REG   0x000F
// Since the module has 7 channels, read data can be up to 14 bytes.
// Plus 1 byte data size and 2 CRC bytes.
#define BUFFER_SIZE 17
#define ID_MAX 247

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

using namespace std::chrono_literals;

// CRC calculation
unsigned short calculateCRC(unsigned char *ptr, int len) {
    unsigned short crc = 0xFFFF;
    while (len--) {
        crc = (crc >> 8) ^ crc_table[(crc ^ *ptr++) & 0xff];
    }
    return (crc);
}

bool isValid(uint8_t ch) { return (ch >= 1 && ch <= CH_MAX); }

int R4AVA07::send(uint8_t rs485_addr, uint8_t func, uint32_t data) {
    uint8_t request[8];
    uint8_t response[BUFFER_SIZE];
    int read_size;

    request[0] = rs485_addr;
    request[1] = func;
    *(uint32_t *)(&request[2]) = htonl(data);
    *(uint16_t *)(&request[6]) = calculateCRC(&request[0], 6);

    write(fd, request, sizeof(request));
    std::this_thread::sleep_for(1s);
    
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

    if (read_size > 0 && func == MODBUS_READ) {
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
    }
    else if (read_size > 0 && func == MODBUS_WRITE) {
        uint16_t *set_value = (uint16_t *)(&request[4]);
        uint16_t *reg_value = (uint16_t *)(&response[4]);
        if(*set_value != *reg_value) {
            return -1;
        }
    }
    return read_size;
}

int R4AVA07::connect(const char *port) {
    fd = open(port, O_RDWR | O_NOCTTY | O_NDELAY);
    if (fd < 0) {
        DEBUG_PRINT("Open port failed: " << errno);
        return -1;
    }

    // Find ID
    send(BROADCAST, MODBUS_READ, ADDR_REG << 16 | 0x01);
    ID = read_data[0];
    if (!ID) { // ID not found.
        DEBUG_PRINT("Cannot find device address.");
        return -1;
    } else DEBUG_PRINT("Found at: " << ID);

    // Find baud rate
    send(ID, MODBUS_WRITE, BAUD_REG << 16 | 0x01);
    baud = read_data[0];
    return fd;
}

std::vector<float>  R4AVA07::readVoltage(uint32_t ch, uint8_t number) {
    if (isValid(ch) == false) {
        DEBUG_PRINT("Invalid channel: " << ch);
        return {-1};
    }
    if (isValid(ch + number -1) == false) {
        DEBUG_PRINT("Invalid read number: " << number);
        return {-1};
    }
    // Channel 1-7 indicated at 0x0000-0x0006.
    if (send(ID, MODBUS_READ, (ch-1) << 16 | number) < 1) {
        DEBUG_PRINT("Cannot read voltage values.");
        return {-1};
    }
    std::vector<float> voltage;
    for (auto i = 0; i < number; i++) {
        voltage.push_back((float) read_data[i] / 100.0);
    }
    return voltage;
}

std::vector<float> R4AVA07::getVoltageRatio(uint32_t ch,
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
    if (send(ID, MODBUS_READ, (ch+6) << 16 | number) < 1) {
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
    
    if (send(ID, MODBUS_WRITE, ADDR_REG << 16 | newID) < 0) {
        DEBUG_PRINT("Cannot set ID.");
        return -1;
    }
    
    ID = newID;
    return 0;
}

int R4AVA07::setVoltageRatio(short ch, float ratio) {
    // Channel 1-7 indicated at 0x0007-0x000D.
    uint16_t val = ratio * 1000;
    if (send(ID, MODBUS_WRITE, (uint32_t) (ch+6) << 16 | val) < 0) {
        DEBUG_PRINT("Cannot set voltage ratio");
        return -1;
    }
    return 0;
}

int R4AVA07::setBaudRate(uint16_t target_baud) {
    uint16_t baud_code;
    switch (target_baud) {
    case 1200:  baud_code = 0; break;
    case 2400:  baud_code = 1; break;
    case 4800:  baud_code = 2; break;
    case 9600:  baud_code = 3; break;
    case 19200: baud_code = 4; break;
    default:
        DEBUG_PRINT("Not supported baud rate");
        return -1;
    }

    if (send(ID, MODBUS_WRITE, BAUD_REG << 16 | baud_code) < 1) {
        DEBUG_PRINT("Cannot change baud rate.");
    }

    baud = target_baud;
    return 0;
}

void R4AVA07::resetBaud() {
    send(ID, MODBUS_WRITE, BAUD_REG << 16 | 0x05);
}