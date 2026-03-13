/**
 * @file timer.h
 * @brief 定时器驱动头文件
 * @details 提供基于ESP32定时器的功能，用于OLED跳秒和EEPROM存储
 */

#ifndef TIMER_H
#define TIMER_H

#include <stdint.h>
#include <stdbool.h>
#include "esp_err.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

// 定时器周期定义
#define TIMER_PERIOD_SEC 1  // 定时器周期1秒

// 定时器事件类型
typedef enum {
    TIMER_EVENT_NONE = 0,       // 无事件
    TIMER_EVENT_TICK = 1,        // 每秒跳变事件
    TIMER_EVENT_SAVE = 2,        // EEPROM保存事件
} TimerEventType;

// 定时器回调函数类型
typedef void (*TimerCallback_t)(TimerEventType event);

/**
 * @brief 定时器初始化函数
 * @details 初始化定时器，设置周期和回调函数，创建事件处理任务
 *
 * @param callback 定时器回调函数指针
 * @return ESP_OK 成功
 *         ESP_FAIL 失败
 */
esp_err_t Timer_Init(TimerCallback_t callback);

/**
 * @brief 启动定时器
 * @details 启动定时器，开始计时
 *
 * @return ESP_OK 成功
 *         ESP_FAIL 失败
 */
esp_err_t Timer_Start(void);

/**
 * @brief 停止定时器
 * @details 停止定时器，暂停计时
 *
 * @return ESP_OK 成功
 *         ESP_FAIL 失败
 */
esp_err_t Timer_Stop(void);

/**
 * @brief 获取定时器运行状态
 * @details 获取定时器是否正在运行
 *
 * @return true 运行中
 *         false 已停止
 */
bool Timer_IsRunning(void);

/**
 * @brief 获取运行时间
 * @details 获取定时器运行的总时间（秒）
 *
 * @return 运行时间（秒）
 */
uint32_t Timer_GetRunTime(void);

/**
 * @brief 重置运行时间
 * @details 将运行时间清零
 */
void Timer_ResetRunTime(void);

/**
 * @brief 设置EEPROM保存间隔
 * @details 设置EEPROM自动保存的时间间隔（秒）
 *
 * @param interval 保存间隔（秒），0表示不自动保存
 */
void Timer_SetSaveInterval(uint32_t interval);

/**
 * @brief 获取EEPROM保存间隔
 * @details 获取当前设置的EEPROM保存间隔
 *
 * @return 保存间隔（秒）
 */
uint32_t Timer_GetSaveInterval(void);

/**
 * @brief 反初始化定时器
 * @details 停止定时器并清理资源
 *
 * @return ESP_OK 成功
 *         ESP_FAIL 失败
 */
esp_err_t Timer_Deinit(void);

#endif // TIMER_H
