/**
 * @file eeprom.c
 * @brief EEPROM存储器驱动实现
 * @details 提供基于AT24C32 EEPROM的驱动功能，用于存储小车运行数据
 */

#include "eeprom.h"
#include "i2c.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <string.h>

static const char *TAG = "EEPROM";

// EEPROM写入周期时间(毫秒)
#define EEPROM_WRITE_CYCLE_MS 5

// 小车数据存储在EEPROM的起始地址
#define CAR_DATA_ADDR 0x0000

// 小车数据存储的魔数，用于验证数据有效性
#define CAR_DATA_MAGIC 0xA5A5

/**
 * @brief 等待EEPROM写入完成
 * @details EEPROM写入操作需要一定时间，此函数等待写入完成
 */
static void EEPROM_WaitForWriteComplete(void)
{
    // 等待EEPROM写入周期完成
    vTaskDelay(pdMS_TO_TICKS(EEPROM_WRITE_CYCLE_MS));
}

/**
 * @brief EEPROM初始化函数
 * @details 初始化EEPROM，检查设备是否可用
 */
void EEPROM_Init(void)
{
    uint8_t test_data = 0xAA;
    uint8_t read_data;

    // 尝试写入测试数据
    if (EEPROM_WriteByte(0x0000, test_data)) {
        // 读取测试数据
        read_data = EEPROM_ReadByte(0x0000);

        if (read_data == test_data) {
            ESP_LOGI(TAG, "EEPROM初始化成功");
        } else {
            ESP_LOGE(TAG, "EEPROM初始化失败: 读写数据不匹配");
        }
    } else {
        ESP_LOGE(TAG, "EEPROM初始化失败: 写入错误");
    }
}

/**
 * @brief EEPROM写入数据
 * @details 向指定地址写入数据，自动处理页边界
 * 
 * @param addr EEPROM内部地址
 * @param data 要写入的数据
 * @param len 数据长度
 * @return true 写入成功
 * @return false 写入失败
 */
bool EEPROM_Write(uint16_t addr, const uint8_t *data, uint16_t len)
{
    uint16_t i;
    uint16_t page_size = 32;  // AT24C32页大小为32字节
    uint16_t offset;
    uint16_t write_len;

    if (addr + len > EEPROM_SIZE) {
        ESP_LOGE(TAG, "EEPROM写入地址超出范围: 0x%04X + %d > %d", addr, len, EEPROM_SIZE);
        return false;
    }

    for (i = 0; i < len; i += page_size) {
        offset = addr % page_size;
        write_len = (page_size - offset) < (len - i) ? (page_size - offset) : (len - i);

        // 构造写入命令：设备地址 + 高位地址 + 低位地址 + 数据
        uint8_t addr_high = (addr + i) >> 8;
        uint8_t addr_low = (addr + i) & 0xFF;

        // 使用16位地址I2C函数写入数据
        if (!I2C_WriteBytes16(EEPROM_ADDRESS, addr_high, addr_low, (uint8_t*)(data + i), write_len)) {
            ESP_LOGE(TAG, "EEPROM写入数据失败");
            return false;
        }

        // 等待写入完成
        EEPROM_WaitForWriteComplete();
    }

    return true;
}

/**
 * @brief EEPROM读取数据
 * @details 从指定地址读取数据
 * 
 * @param addr EEPROM内部地址
 * @param data 存储读取数据的缓冲区
 * @param len 要读取的数据长度
 * @return true 读取成功
 * @return false 读取失败
 */
bool EEPROM_Read(uint16_t addr, uint8_t *data, uint16_t len)
{
    uint8_t addr_high = addr >> 8;
    uint8_t addr_low = addr & 0xFF;

    if (addr + len > EEPROM_SIZE) {
        ESP_LOGE(TAG, "EEPROM读取地址超出范围: 0x%04X + %d > %d", addr, len, EEPROM_SIZE);
        return false;
    }

    // 使用16位地址I2C函数读取数据
    if (!I2C_ReadBytes16(EEPROM_ADDRESS, addr_high, addr_low, data, len)) {
        ESP_LOGE(TAG, "EEPROM读取数据失败");
        return false;
    }

    return true;
}

/**
 * @brief EEPROM写入单个字节
 * @details 向指定地址写入一个字节
 * 
 * @param addr EEPROM内部地址
 * @param data 要写入的数据
 * @return true 写入成功
 * @return false 写入失败
 */
bool EEPROM_WriteByte(uint16_t addr, uint8_t data)
{
    return EEPROM_Write(addr, &data, 1);
}

/**
 * @brief EEPROM读取单个字节
 * @details 从指定地址读取一个字节
 * 
 * @param addr EEPROM内部地址
 * @return uint8_t 读取到的数据，如果失败返回0xFF
 */
uint8_t EEPROM_ReadByte(uint16_t addr)
{
    uint8_t data = 0xFF;
    if (EEPROM_Read(addr, &data, 1)) {
        return data;
    }
    return 0xFF;
}

/**
 * @brief 保存小车运行数据到EEPROM
 * @details 将小车运行数据结构保存到EEPROM
 * 
 * @param data 小车运行数据结构指针
 * @return true 保存成功
 * @return false 保存失败
 */
bool EEPROM_SaveCarData(const CarData *data)
{
    if (data == NULL) {
        ESP_LOGE(TAG, "保存小车数据失败: 数据指针为空");
        return false;
    }

    // 将数据结构转换为字节数组
    uint8_t buffer[sizeof(CarData)];
    memcpy(buffer, data, sizeof(CarData));

    // 写入EEPROM
    if (EEPROM_Write(CAR_DATA_ADDR, buffer, sizeof(CarData))) {
        ESP_LOGI(TAG, "小车数据保存成功");
        return true;
    } else {
        ESP_LOGE(TAG, "小车数据保存失败");
        return false;
    }
}

/**
 * @brief 从EEPROM读取小车运行数据
 * @details 从EEPROM读取小车运行数据结构
 * 
 * @param data 小车运行数据结构指针
 * @return true 读取成功
 * @return false 读取失败
 */
bool EEPROM_LoadCarData(CarData *data)
{
    if (data == NULL) {
        ESP_LOGE(TAG, "读取小车数据失败: 数据指针为空");
        return false;
    }

    // 从EEPROM读取数据
    uint8_t buffer[sizeof(CarData)];
    if (EEPROM_Read(CAR_DATA_ADDR, buffer, sizeof(CarData))) {
        // 将字节数组转换为数据结构
        memcpy(data, buffer, sizeof(CarData));
        ESP_LOGI(TAG, "小车数据读取成功");
        return true;
    } else {
        ESP_LOGE(TAG, "小车数据读取失败");
        return false;
    }
}

/**
 * @brief 清除EEPROM中的小车运行数据
 * @details 将小车运行数据结构清零并保存到EEPROM
 * 
 * @return true 清除成功
 * @return false 清除失败
 */
bool EEPROM_ClearCarData(void)
{
    CarData data = {0};

    if (EEPROM_SaveCarData(&data)) {
        ESP_LOGI(TAG, "小车数据清除成功");
        return true;
    } else {
        ESP_LOGE(TAG, "小车数据清除失败");
        return false;
    }
}
