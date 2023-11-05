#ifndef MODBUS_RTU_H
#define MODBUS_RTU_H
#include <cstdint>
unsigned short calculateCRC(unsigned char *ptr, int len);

enum ModbusFunctionCode {
  ReadCoils            = 0x01,
  ReadDiscreteInputs   = 0x02,
  ReadHoldingRegisters = 0x03,
  ReadInputRegisters   = 0x04,
  WriteSingleCoil      = 0x05,
  WriteSingleRegister  = 0x06,
  WriteMultipleCoils   = 0x0F,
  WriteMultipleRegisters = 0x10
};

#ifdef DEBUG
#include <cerrno>
#include <cstring>
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

#endif

#define BUFFER_SIZE 256

class ModbusDevice {
  private:
    int fd;
    uint8_t response[BUFFER_SIZE];

  protected:
    virtual ~ModbusDevice();
    int connect(const char *port);
    template<unsigned int N>
    int send(uint8_t rs485_addr, ModbusFunctionCode func, const uint8_t (&data)[N]);

  public:
    int sendRead(uint8_t rs485_addr, uint16_t reg_addr,
                 uint16_t count, uint16_t *read_data);
    int sendReadInput(uint8_t rs485_addr, uint16_t reg_addr,
                      uint16_t count, uint16_t *read_data);
    int sendWrite(uint8_t rs485_addr, uint16_t reg_addr, uint16_t value);
    int sendWriteMultiple(uint8_t rs485_addr, uint16_t reg_addr, uint16_t value);
};