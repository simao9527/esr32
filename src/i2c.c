/**
 * @file i2c.c
 * @brief I2C通信驱动实现
 * @details 提供基于ESP32的I2C通信功能，用于OLED和EEPROM等外设的通信
 */

#include "i2c.h"
#include "driver/i2c.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "I2C";

/**
 * @brief I2C初始化函数
 * @details 初始化I2C主机模式，配置SCL和SDA引脚，设置通信频率
 */
void I2C_Init(void)
{
    i2c_config_t conf = {
        .mode = I2C_MODE_MASTER,                    // 主机模式
        .sda_io_num = I2C_SDA_PIN,                  // SDA引脚
        .sda_pullup_en = GPIO_PULLUP_ENABLE,        // 启用SDA上拉
        .scl_io_num = I2C_SCL_PIN,                  // SCL引脚
        .scl_pullup_en = GPIO_PULLUP_ENABLE,        // 启用SCL上拉
        .master.clk_speed = I2C_FREQ_HZ,            // 时钟频率
    };

    i2c_param_config(I2C_NUM_0, &conf);             // 配置I2C参数
    i2c_driver_install(I2C_NUM_0, conf.mode, 0, 0, ESP_INTR_FLAG_LEVEL1); // 安装I2C驱动

    ESP_LOGI(TAG, "I2C初始化完成，SCL: %d, SDA: %d, 频率: %dHz", 
             I2C_SCL_PIN, I2C_SDA_PIN, I2C_FREQ_HZ);
}

/**
 * @brief I2C写入数据
 * @details 向指定设备的指定寄存器写入数据
 * 
 * @param addr 设备地址
 * @param reg 寄存器地址
 * @param data 要写入的数据
 * @param len 数据长度
 * @return true 写入成功
 * @return false 写入失败
 */
bool I2C_WriteBytes(uint8_t addr, uint8_t reg, uint8_t *data, uint16_t len)
{
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (addr << 1) | I2C_MASTER_WRITE, true);
    i2c_master_write_byte(cmd, reg, true);
    i2c_master_write(cmd, data, len, true);
    i2c_master_stop(cmd);

    esp_err_t ret = i2c_master_cmd_begin(I2C_NUM_0, cmd, 1000 / portTICK_PERIOD_MS);
    i2c_cmd_link_delete(cmd);

    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "I2C写入失败，设备地址: 0x%02X, 寄存器: 0x%02X, 错误: %s", 
                 addr, reg, esp_err_to_name(ret));
        return false;
    }

    return true;
}

/**
 * @brief I2C读取数据
 * @details 从指定设备的指定寄存器读取数据
 * 
 * @param addr 设备地址
 * @param reg 寄存器地址
 * @param data 存储读取数据的缓冲区
 * @param len 要读取的数据长度
 * @return true 读取成功
 * @return false 读取失败
 */
bool I2C_ReadBytes(uint8_t addr, uint8_t reg, uint8_t *data, uint16_t len)
{
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (addr << 1) | I2C_MASTER_WRITE, true);
    i2c_master_write_byte(cmd, reg, true);
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (addr << 1) | I2C_MASTER_READ, true);
    if (len > 1) {
        i2c_master_read(cmd, data, len - 1, I2C_MASTER_ACK);
    }
    i2c_master_read_byte(cmd, data + len - 1, I2C_MASTER_NACK);
    i2c_master_stop(cmd);

    esp_err_t ret = i2c_master_cmd_begin(I2C_NUM_0, cmd, 1000 / portTICK_PERIOD_MS);
    i2c_cmd_link_delete(cmd);

    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "I2C读取失败，设备地址: 0x%02X, 寄存器: 0x%02X, 错误: %s", 
                 addr, reg, esp_err_to_name(ret));
        return false;
    }

    return true;
}

/**
 * @brief I2C写入单个字节
 * @details 向指定设备的指定寄存器写入一个字节
 * 
 * @param addr 设备地址
 * @param reg 寄存器地址
 * @param data 要写入的数据
 * @return true 写入成功
 * @return false 写入失败
 */
bool I2C_WriteByte(uint8_t addr, uint8_t reg, uint8_t data)
{
    return I2C_WriteBytes(addr, reg, &data, 1);
}

/**
 * @brief I2C读取单个字节
 * @details 从指定设备的指定寄存器读取一个字节
 * 
 * @param addr 设备地址
 * @param reg 寄存器地址
 * @return uint8_t 读取到的数据，如果失败返回0xFF
 */
uint8_t I2C_ReadByte(uint8_t addr, uint8_t reg)
{
    uint8_t data = 0xFF;
    if (I2C_ReadBytes(addr, reg, &data, 1)) {
        return data;
    }
    return 0xFF;
}

/**
 * @brief I2C写入数据(16位地址)
 * @details 向指定设备的指定16位地址写入数据
 * 
 * @param addr 设备地址
 * @param reg_high 寄存器地址高位
 * @param reg_low 寄存器地址低位
 * @param data 要写入的数据
 * @param len 数据长度
 * @return true 写入成功
 * @return false 写入失败
 */
bool I2C_WriteBytes16(uint8_t addr, uint8_t reg_high, uint8_t reg_low, uint8_t *data, uint16_t len)
{
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (addr << 1) | I2C_MASTER_WRITE, true);
    i2c_master_write_byte(cmd, reg_high, true);  // 写入高位地址
    i2c_master_write_byte(cmd, reg_low, true);   // 写入低位地址
    i2c_master_write(cmd, data, len, true);
    i2c_master_stop(cmd);

    esp_err_t ret = i2c_master_cmd_begin(I2C_NUM_0, cmd, 1000 / portTICK_PERIOD_MS);
    i2c_cmd_link_delete(cmd);

    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "I2C写入失败(16位地址)，设备地址: 0x%02X, 地址: 0x%02X%02X, 错误: %s", 
                 addr, reg_high, reg_low, esp_err_to_name(ret));
        return false;
    }

    return true;
}

/**
 * @brief I2C读取数据(16位地址)
 * @details 从指定设备的指定16位地址读取数据
 * 
 * @param addr 设备地址
 * @param reg_high 寄存器地址高位
 * @param reg_low 寄存器地址低位
 * @param data 存储读取数据的缓冲区
 * @param len 要读取的数据长度
 * @return true 读取成功
 * @return false 读取失败
 */
bool I2C_ReadBytes16(uint8_t addr, uint8_t reg_high, uint8_t reg_low, uint8_t *data, uint16_t len)
{
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (addr << 1) | I2C_MASTER_WRITE, true);
    i2c_master_write_byte(cmd, reg_high, true);  // 写入高位地址
    i2c_master_write_byte(cmd, reg_low, true);   // 写入低位地址
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (addr << 1) | I2C_MASTER_READ, true);
    if (len > 1) {
        i2c_master_read(cmd, data, len - 1, I2C_MASTER_ACK);
    }
    i2c_master_read_byte(cmd, data + len - 1, I2C_MASTER_NACK);
    i2c_master_stop(cmd);

    esp_err_t ret = i2c_master_cmd_begin(I2C_NUM_0, cmd, 1000 / portTICK_PERIOD_MS);
    i2c_cmd_link_delete(cmd);

    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "I2C读取失败(16位地址)，设备地址: 0x%02X, 地址: 0x%02X%02X, 错误: %s", 
                 addr, reg_high, reg_low, esp_err_to_name(ret));
        return false;
    }

    return true;
}

/**
 * @brief I2C写入数据(无寄存器地址)
 * @details 向指定设备写入数据，不发送寄存器地址
 * 
 * @param addr 设备地址
 * @param data 要写入的数据
 * @param len 数据长度
 * @return true 写入成功
 * @return false 写入失败
 */
bool I2C_WriteBytesNoReg(uint8_t addr, uint8_t *data, uint16_t len)
{
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (addr << 1) | I2C_MASTER_WRITE, true);
    i2c_master_write(cmd, data, len, true);
    i2c_master_stop(cmd);

    esp_err_t ret = i2c_master_cmd_begin(I2C_NUM_0, cmd, 1000 / portTICK_PERIOD_MS);
    i2c_cmd_link_delete(cmd);

    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "I2C写入失败(无寄存器地址)，设备地址: 0x%02X, 错误: %s", 
                 addr, esp_err_to_name(ret));
        return false;
    }

    return true;
}
