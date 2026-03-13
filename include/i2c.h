
#ifndef I2C_H
#define I2C_H

#include <stdint.h>
#include <stdbool.h>

// I2C引脚定义
#define I2C_SDA_PIN 8
#define I2C_SCL_PIN 9

// I2C设备地址定义
#define OLED_ADDRESS 0x3C    // OLED显示屏I2C地址
#define EEPROM_ADDRESS 0x50  // EEPROM存储器I2C地址

// I2C通信速度定义
#define I2C_FREQ_HZ 100000   // I2C通信频率100kHz

// I2C初始化函数
void I2C_Init(void);

// I2C写入数据
// 参数:
//   addr - 设备地址
//   reg - 寄存器地址
//   data - 要写入的数据
//   len - 数据长度
// 返回值:
//   true - 写入成功
//   false - 写入失败
bool I2C_WriteBytes(uint8_t addr, uint8_t reg, uint8_t *data, uint16_t len);

// I2C读取数据
// 参数:
//   addr - 设备地址
//   reg - 寄存器地址
//   data - 存储读取数据的缓冲区
//   len - 要读取的数据长度
// 返回值:
//   true - 读取成功
//   false - 读取失败
bool I2C_ReadBytes(uint8_t addr, uint8_t reg, uint8_t *data, uint16_t len);

// I2C写入单个字节
// 参数:
//   addr - 设备地址
//   reg - 寄存器地址
//   data - 要写入的数据
// 返回值:
//   true - 写入成功
//   false - 写入失败
bool I2C_WriteByte(uint8_t addr, uint8_t reg, uint8_t data);

// I2C读取单个字节
// 参数:
//   addr - 设备地址
//   reg - 寄存器地址
// 返回值:
//   读取到的数据，如果失败返回0xFF
uint8_t I2C_ReadByte(uint8_t addr, uint8_t reg);

// I2C写入数据(16位地址)
// 参数:
//   addr - 设备地址
//   reg_high - 寄存器地址高位
//   reg_low - 寄存器地址低位
//   data - 要写入的数据
//   len - 数据长度
// 返回值:
//   true - 写入成功
//   false - 写入失败
bool I2C_WriteBytes16(uint8_t addr, uint8_t reg_high, uint8_t reg_low, uint8_t *data, uint16_t len);

// I2C读取数据(16位地址)
// 参数:
//   addr - 设备地址
//   reg_high - 寄存器地址高位
//   reg_low - 寄存器地址低位
//   data - 存储读取数据的缓冲区
//   len - 要读取的数据长度
// 返回值:
//   true - 读取成功
//   false - 读取失败
bool I2C_ReadBytes16(uint8_t addr, uint8_t reg_high, uint8_t reg_low, uint8_t *data, uint16_t len);

// I2C写入数据(无寄存器地址)
// 参数:
//   addr - 设备地址
//   data - 要写入的数据
//   len - 数据长度
// 返回值:
//   true - 写入成功
//   false - 写入失败
bool I2C_WriteBytesNoReg(uint8_t addr, uint8_t *data, uint16_t len);

#endif // I2C_H
