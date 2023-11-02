#ifndef AMVIF08_H
#define AMVIF08_H
#include <vector>
#include <cstdint>
#endif
#define R4AVA07LIB_VERSION "1.0.0"

#ifndef CRC16_H
#include "../include/crc16.hpp"
#endif

enum class Modbus;
enum class Register;

class AMVIF08 {
  private:
    int fd;
    uint16_t read_data[8] = {0x00};

    unsigned short prod_id = 2408;
    short return_time = 1000;
    uint8_t addr;
    short baudrate = 9600;
    char parity = 'N';

  protected:
    // Send command to AMVIF08 and read returned message in buffer
    int send(uint8_t rs485_addr, Modbus func, uint32_t data);
    // Check if channel is in range 1-8
    bool isValid(short ch);
    
  public:
    int connect(const char* port);
    // Read channel's voltage 
    std::vector<float> readVoltage(uint32_t ch,
                                   uint8_t number = 0x01);
    // Return channel's voltage ratio
    std::vector<float> getVoltageRatio(uint32_t ch,
                                       uint8_t number = 0x01);
    // Return product's ID
    unsigned short getProductID() { return prod_id; }
    // Return time interal for response in ms
    short getReturnTime()         { return return_time; };
    // Return slave's address
    uint8_t getAddr()             { return addr; }
    // Return current baud rate
    short getBaud()               { return baudrate; }
    // Return parity type
    char getParity()              { return parity; }

    // Set channel's voltage ratio
    short setVoltageRatio(short ch, float ratio);
    // Factory reset
    short factoryReset();
    // Set time interval for command return
    short setReturnTime(short msec);
    // Set slave's address
    short setAddr(short new_addr);
    // Change serial  baud rate
    short setBaudRate(short baud = 9600);
    // Change parity check type
    short setParity(short type = 0);
};