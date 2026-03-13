/**
 * @file xunji.h
 * @brief 五路寻迹传感器驱动头文件
 * @details 提供基于五路红外传感器的寻迹功能，支持读取传感器状态和判断轨迹位置
 */

#ifndef XUNJI_H
#define XUNJI_H

#include <stdint.h>
#include <stdbool.h>
#include "driver/gpio.h"

// 寻迹传感器引脚定义
#define XUNJI_SENSOR1_PIN 10  // 最左侧传感器
#define XUNJI_SENSOR2_PIN 11  // 左侧传感器
#define XUNJI_SENSOR3_PIN 12  // 中间传感器
#define XUNJI_SENSOR4_PIN 13  // 右侧传感器
#define XUNJI_SENSOR5_PIN 14  // 最右侧传感器

// 寻迹传感器数量
#define XUNJI_SENSOR_COUNT 5

// 寻迹传感器阈值定义
#define XUNJI_WHITE_LINE 0    // 白线（低电平）
#define XUNJI_BLACK_LINE 1    // 黑线（高电平）

// 寻迹位置枚举
typedef enum {
    XUNJI_POS_LOST = -1,      // 丢失轨迹
    XUNJI_POS_FAR_LEFT = 0,   // 最左侧
    XUNJI_POS_LEFT = 1,       // 左侧
    XUNJI_POS_CENTER = 2,     // 中间
    XUNJI_POS_RIGHT = 3,      // 右侧
    XUNJI_POS_FAR_RIGHT = 4   // 最右侧
} XunjiPosition;

// 寻迹传感器状态结构体
typedef struct {
    bool sensor1;  // 最左侧传感器状态
    bool sensor2;  // 左侧传感器状态
    bool sensor3;  // 中间传感器状态
    bool sensor4;  // 右侧传感器状态
    bool sensor5;  // 最右侧传感器状态
} XunjiSensorState;

// 寻迹传感器配置结构体
typedef struct {
    gpio_num_t pins[XUNJI_SENSOR_COUNT];  // 传感器引脚数组
    bool active_level;                    // 有效电平（0为低电平有效，1为高电平有效）
} XunjiConfig;

/**
 * @brief 寻迹传感器初始化函数
 * @details 初始化寻迹传感器的GPIO引脚
 */
void Xunji_Init(void);

/**
 * @brief 读取寻迹传感器状态
 * @details 读取所有传感器的当前状态
 *
 * @return 寻迹传感器状态结构体
 */
XunjiSensorState Xunji_ReadSensors(void);

/**
 * @brief 获取轨迹位置
 * @details 根据传感器状态判断轨迹相对于车辆的位置
 *
 * @return 轨迹位置枚举值
 */
XunjiPosition Xunji_GetPosition(void);

/**
 * @brief 判断是否丢失轨迹
 * @details 判断是否所有传感器都没有检测到轨迹
 *
 * @return true表示丢失轨迹，false表示检测到轨迹
 */
bool Xunji_IsLineLost(void);

/**
 * @brief 获取轨迹偏移量
 * @details 计算轨迹相对于中心的偏移量，用于PID控制
 *
 * @return 偏移量，负值表示偏左，正值表示偏右，0表示居中
 */
int8_t Xunji_GetOffset(void);

/**
 * @brief 获取单个传感器状态
 * @details 获取指定传感器的状态
 *
 * @param index 传感器索引（0-4）
 * @return 传感器状态，true表示检测到线，false表示未检测到线
 */
bool Xunji_GetSensor(uint8_t index);

#endif // XUNJI_H
