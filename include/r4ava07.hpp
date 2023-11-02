#ifndef R4AVA07_H
#define R4AVA07_H
#include <vector>
#include <cstdint>
#endif
#define R4AVA07LIB_VERSION "1.0.0"

#ifndef CRC16_H
#include "../include/crc16.hpp"
#endif

class R4AVA07 {
  private:
    short ID;
    short baud;
    int fd;
    uint8_t read_data[7] = {0x00};

  protected:
    // Send command to R4AVA07 and read returned message in buffer
    int send(uint8_t rs485_addr, uint8_t func, uint32_t data);
    // Check channel range
    bool isValid(short ch);

  public:
    int connect(const char* port);
    // Return slave's ID
    short getID()   { return ID; };
    // Return current baud rate
    short getBaud() { return baud; };
    // Read channel's voltage 
    std::vector<float> readVoltage(uint32_t ch, uint8_t number = 0x01);
    // Return channel's voltage ratio
    std::vector<float> getVoltageRatio(uint32_t ch, uint8_t number = 0x01);

    // Set slave's ID
    int setID(short new_id);
    // Set channel's voltage ratio
    int setVoltageRatio(short ch, float val);
    // Change serial  baud rate
    int setBaudRate(uint16_t baud);
    // Reset serial baud rate
    void resetBaud();
};