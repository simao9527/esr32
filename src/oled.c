/**
 * @file oled.c
 * @brief OLED显示屏驱动实现
 * @details 提供基于SSD1306 OLED显示屏的驱动功能，支持显示字符、字符串、数字、汉字和图像
 */

#include "oled.h"
#include "i2c.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <string.h>

static const char *TAG = "OLED";

// OLED显示缓冲区
static uint8_t OLED_Buffer[OLED_HEIGHT * OLED_WIDTH / 8];

// OLED命令定义
#define OLED_CMD 0x00
#define OLED_DATA 0x40

// 常用ASCII字符字库 (8x16字体)
static const uint8_t ASCII_8x16[][16] = {
    // 这里只包含部分字符作为示例，实际应用中需要完整的ASCII字库
    {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00},// 空格
    {0x00,0x00,0x00,0xF8,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x33,0x30,0x00,0x00,0x00},// !
    // 其他字符...
};

// 6x8字体
static const uint8_t ASCII_6x8[][6] = {
    {0x00,0x00,0x00,0x00,0x00,0x00},// 空格
    {0x00,0x00,0x2F,0x00,0x00,0x00},// !
    // 其他字符...
};

/**
 * @brief 向OLED写入命令
 * @param cmd 要写入的命令
 */
static void OLED_WriteCommand(uint8_t cmd)
{
    uint8_t buf[2] = {OLED_CMD, cmd};
    I2C_WriteBytesNoReg(OLED_ADDRESS, buf, 2);
}

/**
 * @brief 向OLED写入数据
 * @param data 要写入的数据
 */
static void OLED_WriteData(uint8_t data)
{
    uint8_t buf[2] = {OLED_DATA, data};
    I2C_WriteBytesNoReg(OLED_ADDRESS, buf, 2);
}

/**
 * @brief OLED初始化函数
 * @details 初始化OLED显示屏，设置显示参数
 */
void OLED_Init(void)
{
    // 等待OLED上电稳定
    vTaskDelay(pdMS_TO_TICKS(100));

    // 初始化OLED
    OLED_WriteCommand(0xAE); // 关闭显示

    OLED_WriteCommand(0xD5); // 设置时钟分频因子
    OLED_WriteCommand(0x80); // 建议值

    OLED_WriteCommand(0xA8); // 设置驱动路数
    OLED_WriteCommand(0x3F); // 默认0X3F(1/64)

    OLED_WriteCommand(0xD3); // 设置显示偏移
    OLED_WriteCommand(0x00); // 默认为0

    OLED_WriteCommand(0x40); // 设置显示开始行 [5:0],行0

    OLED_WriteCommand(0x8D); // 电荷泵设置
    OLED_WriteCommand(0x14); // 开启电荷泵

    OLED_WriteCommand(0x20); // 设置内存地址模式
    OLED_WriteCommand(0x02); // 页地址模式

    OLED_WriteCommand(0xA1); // 段重定义

    OLED_WriteCommand(0xC8); // 设置COM扫描方向

    OLED_WriteCommand(0xDA); // 设置COM硬件引脚配置
    OLED_WriteCommand(0x12);

    OLED_WriteCommand(0x81); // 对比度设置
    OLED_WriteCommand(0xCF); // 默认0x7F

    OLED_WriteCommand(0xD9); // 设置预充电周期
    OLED_WriteCommand(0xF1);

    OLED_WriteCommand(0xDB); // 设置VCOMH电压倍率
    OLED_WriteCommand(0x40);

    OLED_WriteCommand(0xA4); // 全局显示开启

    OLED_WriteCommand(0xA6); // 设置显示方式

    OLED_WriteCommand(0xAF); // 开启显示

    OLED_Clear();

    ESP_LOGI(TAG, "OLED初始化完成");
}

/**
 * @brief OLED清屏函数
 * @details 清空OLED显示缓冲区
 */
void OLED_Clear(void)
{
    memset(OLED_Buffer, 0, sizeof(OLED_Buffer));
}

/**
 * @brief OLED刷新显示
 * @details 将缓冲区内容写入OLED显示屏
 */
/**
 * @brief OLED显示屏刷新函数
 * 该函数用于将OLED_Buffer中的数据刷新到OLED显示屏上
 * 按页依次写入数据，每页包含OLED_WIDTH个像素点
 */
void OLED_Refresh(void)
{
    uint8_t i;          // 循环计数器，用于页地址遍历
    uint8_t buf[OLED_WIDTH + 1];  // 数据缓冲区，用于存放待写入的数据，额外1字节用于数据标识

    // 遍历所有8页（OLED通常为8页显示）
    for (i = 0; i < 8; i++) {
        OLED_WriteCommand(0xB0 + i); // 设置页地址，0xB0为起始页地址，i为当前页
        OLED_WriteCommand(0x00);     // 设置显示位置-低列起始地址（列地址低4位）
        OLED_WriteCommand(0x10);     // 设置显示位置-高列起始地址（列地址高4位）

        // 准备写入一页数据
        buf[0] = OLED_DATA;  // 缓冲区首字节为数据标识，表示接下来是显示数据
        // 将OLED_Buffer中当前页的数据复制到缓冲区，每页宽度为OLED_WIDTH
        memcpy(&buf[1], &OLED_Buffer[i * OLED_WIDTH], OLED_WIDTH);

        // 写入一页数据
        I2C_WriteBytesNoReg(OLED_ADDRESS, buf, OLED_WIDTH + 1);
    }
}

/**
 * @brief 计算幂运算
 * @param m 底数
 * @param n 指数
 * @return uint32_t 计算结果
 */
static uint32_t OLED_Pow(uint8_t m, uint8_t n)
{
    uint32_t result = 1;
    while (n--) {
        result *= m;
    }
    return result;
}

/**
 * @brief OLED绘制像素点
 * @param x 横坐标(0-127)
 * @param y 纵坐标(0-63)
 * @param color 颜色(1-亮, 0-灭)
 */
void OLED_DrawPixel(uint8_t x, uint8_t y, uint8_t color)
{
    if (x >= OLED_WIDTH || y >= OLED_HEIGHT) {
        return;
    }

    uint8_t page = y / 8;
    uint8_t bit = y % 8;

    if (color) {
        OLED_Buffer[page * OLED_WIDTH + x] |= (1 << bit);
    } else {
        OLED_Buffer[page * OLED_WIDTH + x] &= ~(1 << bit);
    }
}

/**
 * @brief OLED绘制字符
 * @param x 起始横坐标
 * @param y 起始纵坐标
 * @param ch 要显示的字符
 * @param size 字体大小(12/16/24)
 */
void OLED_ShowChar(uint8_t x, uint8_t y, char ch, uint8_t size)
{
    uint8_t c = 0;
    uint8_t i = 0;
    uint8_t j = 0;

    c = ch - ' ';

    if (x > OLED_WIDTH - 1) {
        x = 0;
        y = y + 2;
    }

    if (size == 16) {
        for (i = 0; i < 8; i++) {
            for (j = 0; j < 8; j++) {
                if (ASCII_8x16[c][i] & (1 << j)) {
                    OLED_DrawPixel(x + j, y + i, 1);
                } else {
                    OLED_DrawPixel(x + j, y + i, 0);
                }
            }
        }
        for (i = 0; i < 8; i++) {
            for (j = 0; j < 8; j++) {
                if (ASCII_8x16[c][i + 8] & (1 << j)) {
                    OLED_DrawPixel(x + j, y + i + 8, 1);
                } else {
                    OLED_DrawPixel(x + j, y + i + 8, 0);
                }
            }
        }
    } else if (size == 12) {
        // 12号字体实现
    } else if (size == 24) {
        // 24号字体实现
    } else {
        // 默认使用8号字体
        for (i = 0; i < 6; i++) {
            for (j = 0; j < 8; j++) {
                if (ASCII_6x8[c][i] & (1 << j)) {
                    OLED_DrawPixel(x + i, y + j, 1);
                } else {
                    OLED_DrawPixel(x + i, y + j, 0);
                }
            }
        }
    }
}

/**
 * @brief OLED绘制字符串
 * @param x 起始横坐标
 * @param y 起始纵坐标
 * @param str 要显示的字符串
 * @param size 字体大小(12/16/24)
 */
void OLED_ShowString(uint8_t x, uint8_t y, const char *str, uint8_t size)
{
    uint8_t char_width = 8;

    if (size == 12) {
        char_width = 6;
    } else if (size == 16) {
        char_width = 8;
    } else if (size == 24) {
        char_width = 12;
    }

    while (*str != '\0') {
        OLED_ShowChar(x, y, *str, size);
        x += char_width;
        if (x > OLED_WIDTH - char_width) {
            x = 0;
            y += size;
        }
        str++;
    }
}

/**
 * @brief OLED绘制数字
 * @param x 起始横坐标
 * @param y 起始纵坐标
 * @param num 要显示的数字
 * @param len 数字长度
 * @param size 字体大小(12/16/24)
 */
void OLED_ShowNum(uint8_t x, uint8_t y, uint32_t num, uint8_t len, uint8_t size)
{
    uint8_t t, temp;
    uint8_t enshow = 0;

    for (t = 0; t < len; t++) {
        temp = (num / OLED_Pow(10, len - t - 1)) % 10;
        if (enshow == 0 && t < (len - 1)) {
            if (temp == 0) {
                OLED_ShowChar(x + (size / 2) * t, y, ' ', size);
                continue;
            } else {
                enshow = 1;
            }
        }
        OLED_ShowChar(x + (size / 2) * t, y, temp + '0', size);
    }
}

/**
 * @brief OLED绘制浮点数
 * @param x 起始横坐标
 * @param y 起始纵坐标
 * @param num 要显示的浮点数
 * @param len 整数部分长度
 * @param size 字体大小(12/16/24)
 * @param decimal 小数点位数
 */
void OLED_ShowFloat(uint8_t x, uint8_t y, float num, uint8_t len, uint8_t size, uint8_t decimal)
{
    uint32_t int_part = (uint32_t)num;
    float float_part = num - int_part;
    uint32_t decimal_part = (uint32_t)(float_part * OLED_Pow(10, decimal));

    OLED_ShowNum(x, y, int_part, len, size);

    // 显示小数点
    OLED_ShowChar(x + (size / 2) * len, y, '.', size);

    // 显示小数部分
    OLED_ShowNum(x + (size / 2) * (len + 1), y, decimal_part, decimal, size);
}

/**
 * @brief OLED绘制汉字
 * @param x 起始横坐标
 * @param y 起始纵坐标
 * @param index 汉字索引
 */
void OLED_ShowChinese(uint8_t x, uint8_t y, uint8_t index)
{
    // 汉字字库实现
    // 这里需要根据实际使用的汉字字库来实现
}

/**
 * @brief OLED绘制图像
 * @param x 起始横坐标
 * @param y 起始纵坐标
 * @param width 图像宽度
 * @param height 图像高度
 * @param bmp 图像数据
 */
void OLED_DrawBMP(uint8_t x, uint8_t y, uint8_t width, uint8_t height, const uint8_t *bmp)
{
    uint8_t i, j;
    uint8_t page, bit;

    for (i = 0; i < height; i++) {
        for (j = 0; j < width; j++) {
            page = (y + i) / 8;
            bit = (y + i) % 8;

            if (bmp[(i * width + j) / 8] & (1 << ((i * width + j) % 8))) {
                OLED_DrawPixel(x + j, y + i, 1);
            } else {
                OLED_DrawPixel(x + j, y + i, 0);
            }
        }
    }
}

/**
 * @brief OLED显示总运行时间
 * @details 显示总运行时间，格式化为时:分:秒
 *
 * @param x 起始横坐标
 * @param y 起始纵坐标
 * @param total_time 总运行时间(秒)
 * @param size 字体大小(12/16/24)
 */
void OLED_ShowTotalTime(uint8_t x, uint8_t y, uint32_t total_time, uint8_t size)
{
    uint32_t hours = total_time / 3600;
    uint32_t minutes = (total_time % 3600) / 60;
    uint32_t seconds = total_time % 60;

    // 显示"时间:"标签
    OLED_ShowString(x, y, "Time:", size);

    // 显示时:分:秒
    uint8_t offset = 5 * 8; // "Time:"占用的宽度
    if (size == 16) offset = 5 * 8;
    else if (size == 24) offset = 5 * 12;

    OLED_ShowNum(x + offset, y, hours, 2, size);
    OLED_ShowChar(x + offset + 2 * 8, y, ':', size);
    OLED_ShowNum(x + offset + 3 * 8, y, minutes, 2, size);
    OLED_ShowChar(x + offset + 5 * 8, y, ':', size);
    OLED_ShowNum(x + offset + 6 * 8, y, seconds, 2, size);
}

/**
 * @brief OLED显示速度档位
 * @details 显示速度档位和对应的文本描述
 *
 * @param x 起始横坐标
 * @param y 起始纵坐标
 * @param speed_level 速度档位(0-低, 1-中, 2-高)
 * @param size 字体大小(12/16/24)
 */
void OLED_ShowSpeedLevel(uint8_t x, uint8_t y, uint8_t speed_level, uint8_t size)
{
    // 显示"速度:"标签
    OLED_ShowString(x, y, "Speed:", size);

    uint8_t offset = 6 * 8; // "Speed:"占用的宽度
    if (size == 16) offset = 6 * 8;
    else if (size == 24) offset = 6 * 12;

    // 显示速度档位数字
    OLED_ShowNum(x + offset, y, speed_level, 1, size);

    // 显示速度档位描述
    offset += 8;
    const char *speed_text[] = {"Low", "Mid", "High"};
    if (speed_level <= 2) {
        OLED_ShowString(x + offset, y, speed_text[speed_level], size);
    }
}

/**
 * @brief OLED显示避障次数
 * @details 显示避障次数
 *
 * @param x 起始横坐标
 * @param y 起始纵坐标
 * @param obstacle_count 避障次数
 * @param size 字体大小(12/16/24)
 */
void OLED_ShowObstacleCount(uint8_t x, uint8_t y, uint32_t obstacle_count, uint8_t size)
{
    // 显示"避障:"标签
    OLED_ShowString(x, y, "Avoid:", size);

    uint8_t offset = 6 * 8; // "Avoid:"占用的宽度
    if (size == 16) offset = 6 * 8;
    else if (size == 24) offset = 6 * 12;

    // 显示避障次数
    OLED_ShowNum(x + offset, y, obstacle_count, 4, size);
}
