
#ifndef EEPROM_H
#define EEPROM_H

#include <stdint.h>
#include <stdbool.h>

// EEPROM存储器大小定义
#define EEPROM_SIZE 4096  // 32Kbit EEPROM (AT24C32)

// 小车运行数据结构
typedef struct {
    uint32_t total_run_time;    // 总运行时间(秒)
    uint32_t lap_count;         // 圈数
    uint32_t obstacle_count;    // 避障次数
    uint8_t speed_level;        // 速度档位(0-低, 1-中, 2-高)
    uint8_t reserved[3];        // 保留字节
} CarData;

// EEPROM初始化函数
void EEPROM_Init(void);

// EEPROM写入数据
// 参数:
//   addr - EEPROM内部地址
//   data - 要写入的数据
//   len - 数据长度
// 返回值:
//   true - 写入成功
//   false - 写入失败
bool EEPROM_Write(uint16_t addr, const uint8_t *data, uint16_t len);

// EEPROM读取数据
// 参数:
//   addr - EEPROM内部地址
//   data - 存储读取数据的缓冲区
//   len - 要读取的数据长度
// 返回值:
//   true - 读取成功
//   false - 读取失败
bool EEPROM_Read(uint16_t addr, uint8_t *data, uint16_t len);

// EEPROM写入单个字节
// 参数:
//   addr - EEPROM内部地址
//   data - 要写入的数据
// 返回值:
//   true - 写入成功
//   false - 写入失败
bool EEPROM_WriteByte(uint16_t addr, uint8_t data);

// EEPROM读取单个字节
// 参数:
//   addr - EEPROM内部地址
// 返回值:
//   读取到的数据，如果失败返回0xFF
uint8_t EEPROM_ReadByte(uint16_t addr);

// 保存小车运行数据到EEPROM
// 参数:
//   data - 小车运行数据结构指针
// 返回值:
//   true - 保存成功
//   false - 保存失败
bool EEPROM_SaveCarData(const CarData *data);

// 从EEPROM读取小车运行数据
// 参数:
//   data - 小车运行数据结构指针
// 返回值:
//   true - 读取成功
//   false - 读取失败
bool EEPROM_LoadCarData(CarData *data);

// 清除EEPROM中的小车运行数据
// 返回值:
//   true - 清除成功
//   false - 清除失败
bool EEPROM_ClearCarData(void);

#endif // EEPROM_H
