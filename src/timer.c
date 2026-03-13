/**
 * @file timer.c
 * @brief 定时器驱动实现文件
 * @details 提供基于ESP32定时器的功能，用于OLED跳秒和EEPROM存储
 */

#include "timer.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "TIMER";

// 定时器句柄
static esp_timer_handle_t timer_handle = NULL;

// 回调函数指针
static TimerCallback_t timer_callback = NULL;

// 运行时间（秒）
static uint32_t run_time = 0;

// EEPROM保存间隔（秒）
static uint32_t save_interval = 0;

// 上次保存时间（秒）
static uint32_t last_save_time = 0;

// 定时器是否运行
static bool is_running = false;

/**
 * @brief 定时器中断回调函数
 * @details 定时器到期时调用的函数
 */
static void IRAM_ATTR timer_isr_callback(void* arg)
{
    // 增加运行时间
    run_time++;

    // 触发每秒跳变事件
    if (timer_callback != NULL) {
        timer_callback(TIMER_EVENT_TICK);
    }

    // 检查是否需要保存EEPROM
    if (save_interval > 0 && (run_time - last_save_time) >= save_interval) {
        last_save_time = run_time;
        if (timer_callback != NULL) {
            timer_callback(TIMER_EVENT_SAVE);
        }
    }
}

/**
 * @brief 定时器初始化函数
 * @details 初始化定时器，设置周期和回调函数
 *
 * @param callback 定时器回调函数指针
 * @return ESP_OK 成功
 *         ESP_FAIL 失败
 */
esp_err_t Timer_Init(TimerCallback_t callback)
{
    esp_err_t ret;

    // 保存回调函数
    timer_callback = callback;

    // 创建定时器配置
    const esp_timer_create_args_t timer_args = {
        .callback = &timer_isr_callback,
        .name = "periodic_timer"
    };

    // 创建定时器
    ret = esp_timer_create(&timer_args, &timer_handle);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "定时器创建失败: %s", esp_err_to_name(ret));
        return ret;
    }

    ESP_LOGI(TAG, "定时器初始化成功");
    return ESP_OK;
}

/**
 * @brief 启动定时器
 * @details 启动定时器，开始计时
 *
 * @return ESP_OK 成功
 *         ESP_FAIL 失败
 */
esp_err_t Timer_Start(void)
{
    esp_err_t ret;

    if (timer_handle == NULL) {
        ESP_LOGE(TAG, "定时器未初始化");
        return ESP_FAIL;
    }

    if (is_running) {
        ESP_LOGW(TAG, "定时器已经在运行");
        return ESP_OK;
    }

    // 启动定时器，周期为1秒
    ret = esp_timer_start_periodic(timer_handle, TIMER_PERIOD_SEC * 1000000);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "定时器启动失败: %s", esp_err_to_name(ret));
        return ret;
    }

    is_running = true;
    ESP_LOGI(TAG, "定时器已启动");
    return ESP_OK;
}

/**
 * @brief 停止定时器
 * @details 停止定时器，暂停计时
 *
 * @return ESP_OK 成功
 *         ESP_FAIL 失败
 */
esp_err_t Timer_Stop(void)
{
    esp_err_t ret;

    if (timer_handle == NULL) {
        ESP_LOGE(TAG, "定时器未初始化");
        return ESP_FAIL;
    }

    if (!is_running) {
        ESP_LOGW(TAG, "定时器已经停止");
        return ESP_OK;
    }

    // 停止定时器
    ret = esp_timer_stop(timer_handle);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "定时器停止失败: %s", esp_err_to_name(ret));
        return ret;
    }

    is_running = false;
    ESP_LOGI(TAG, "定时器已停止");
    return ESP_OK;
}

/**
 * @brief 获取定时器运行状态
 * @details 获取定时器是否正在运行
 *
 * @return true 运行中
 *         false 已停止
 */
bool Timer_IsRunning(void)
{
    return is_running;
}

/**
 * @brief 获取运行时间
 * @details 获取定时器运行的总时间（秒）
 *
 * @return 运行时间（秒）
 */
uint32_t Timer_GetRunTime(void)
{
    return run_time;
}

/**
 * @brief 重置运行时间
 * @details 将运行时间清零
 */
void Timer_ResetRunTime(void)
{
    run_time = 0;
    last_save_time = 0;
    ESP_LOGI(TAG, "运行时间已重置");
}

/**
 * @brief 设置EEPROM保存间隔
 * @details 设置EEPROM自动保存的时间间隔（秒）
 *
 * @param interval 保存间隔（秒），0表示不自动保存
 */
void Timer_SetSaveInterval(uint32_t interval)
{
    save_interval = interval;
    last_save_time = run_time;
    ESP_LOGI(TAG, "EEPROM保存间隔设置为: %u秒", interval);
}

/**
 * @brief 获取EEPROM保存间隔
 * @details 获取当前设置的EEPROM保存间隔
 *
 * @return 保存间隔（秒）
 */
uint32_t Timer_GetSaveInterval(void)
{
    return save_interval;
}
