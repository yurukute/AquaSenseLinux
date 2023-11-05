#ifndef AMVIF08_H
#define AMVIF08_H
#include <vector>
#include <cstdint>
#endif
#define R4AVA07LIB_VERSION "1.0.0"

#ifndef MODBUS_RTU_H
#include "modbus_rtu.hpp"
#endif

class AMVIF08 : public ModbusDevice {
  private:
    unsigned short prod_id;
    short return_time;
    uint8_t addr;
    short baudrate;
    char parity;

  protected:
    // Check if channel is in range 1-8
    bool isValidChannel(unsigned short ch);

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
    short setVoltageRatio(unsigned short ch, float ratio);
    // Factory reset
    short factoryReset();
    // Set time interval for command return
    short setReturnTime(unsigned short msec);
    // Set slave's address
    short setAddr(unsigned short new_addr);
    // Change serial  baud rate
    short setBaudRate(unsigned short baud = 9600);
    // Change parity check type
    short setParity(unsigned short type = 0);
};
