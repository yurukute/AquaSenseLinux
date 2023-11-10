#ifndef R4AVA07_H
#define R4AVA07_H
#include <cstdint>
#include <vector>
#include <string>
#include <modbus/modbus.h>
#endif
#define R4AVA07LIB_VERSION "1.0.0"

class R4AVA07 {
  private:
    modbus_t *ctx;
    std::string name = "R4AVA07";
    std::string rs485_port;
    short baud;

  protected:
    // Check channel range
    bool isValid(short ch);

  public:
    int connect(const char* port);
    // Return slave's ID
    short getID()   { return modbus_get_slave(ctx); };
    // Return current baud rate
    short getBaud() { return baud; };
    // Read channel's voltage 
    std::vector<float> readVoltage(uint16_t ch, uint8_t number = 0x01);
    // Return channel's voltage ratio
    std::vector<float> getVoltageRatio(uint16_t ch, uint8_t number = 0x01);

    // Set slave's ID
    int setID(short new_id);
    // Set channel's voltage ratio
    int setVoltageRatio(short ch, float val);
    // Change serial  baud rate
    int setBaudRate(uint16_t baud);
    // Reset serial baud rate
    void resetBaud();
};