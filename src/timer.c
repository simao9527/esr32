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
#include "freertos/queue.h"

static const char *TAG = "TIMER";

// 定时器句柄
static esp_timer_handle_t timer_handle = NULL;

// 事件队列句柄
static QueueHandle_t event_queue = NULL;

// 事件处理任务句柄
static TaskHandle_t event_task_handle = NULL;

// 回调函数指针
static TimerCallback_t timer_callback = NULL;

// 运行时间（秒）
static volatile uint32_t run_time = 0;

// EEPROM保存间隔（秒）
static uint32_t save_interval = 0;

// 上次保存时间（秒）
static uint32_t last_save_time = 0;

// 定时器是否运行
static volatile bool is_running = false;

// 事件队列大小
#define EVENT_QUEUE_SIZE 10

/**
 * @brief 定时器中断回调函数
 * @details 定时器到期时调用的函数，只做最简单的操作，通过队列发送事件
 */
static void IRAM_ATTR timer_isr_callback(void* arg)
{
    // 只在中断中增加运行时间，不执行其他操作
    run_time++;

    // 发送每秒跳变事件到队列
    send_timer_event(TIMER_EVENT_TICK);

    // 检查是否需要保存EEPROM
    if (save_interval > 0 && (run_time - last_save_time) >= save_interval) {
        last_save_time = run_time;
        // 发送保存事件到队列
        send_timer_event(TIMER_EVENT_SAVE);
    }
}

/**
 * @brief 事件处理任务
 * @details 在任务中处理定时器事件，避免中断中执行耗时操作
 */
static void event_task(void* arg)
{
    TimerEventType event;

    while (1) {
        // 等待事件
        if (xQueueReceive(event_queue, &event, portMAX_DELAY) == pdTRUE) {
            // 在任务中调用回调函数
            if (timer_callback != NULL) {
                timer_callback(event);
            }
        }
    }

    // 任务不应该到达这里
    vTaskDelete(NULL);
}

/**
 * @brief 定时器初始化函数
 * @details 初始化定时器，设置周期和回调函数，创建事件队列和任务
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

    // 创建事件队列
    event_queue = xQueueCreate(EVENT_QUEUE_SIZE, sizeof(TimerEventType));
    if (event_queue == NULL) {
        ESP_LOGE(TAG, "事件队列创建失败");
        return ESP_FAIL;
    }

    // 创建事件处理任务，优先级设为较低，避免干扰其他任务
    BaseType_t task_ret = xTaskCreate(
        event_task,
        "timer_event",
        2048,              // 栈大小
        NULL,
        1,                 // 优先级（低优先级）
        &event_task_handle
    );

    if (task_ret != pdPASS) {
        ESP_LOGE(TAG, "事件处理任务创建失败");
        vQueueDelete(event_queue);
        event_queue = NULL;
        return ESP_FAIL;
    }

    // 创建定时器配置
    const esp_timer_create_args_t timer_args = {
        .callback = &timer_isr_callback,
        .name = "periodic_timer"
    };

    // 创建定时器
    ret = esp_timer_create(&timer_args, &timer_handle);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "定时器创建失败: %s", esp_err_to_name(ret));
        vTaskDelete(event_task_handle);
        vQueueDelete(event_queue);
        event_task_handle = NULL;
        event_queue = NULL;
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

    if (timer_handle == NULL || event_queue == NULL) {
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
 * @brief 发送定时器事件到队列
 * @details 从定时器回调中调用，将事件发送到队列供任务处理
 *
 * @param event 事件类型
 */
static void send_timer_event(TimerEventType event)
{
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;

    // 尝试发送事件到队列
    if (xQueueSendFromISR(event_queue, &event, &xHigherPriorityTaskWoken) != pdTRUE) {
        // 队列满，丢弃事件
        return;
    }

    // 如果有更高优先级任务被唤醒，进行上下文切换
    if (xHigherPriorityTaskWoken == pdTRUE) {
        portYIELD_FROM_ISR();
    }
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

/**
 * @brief 反初始化定时器
 * @details 停止定时器并清理资源
 *
 * @return ESP_OK 成功
 *         ESP_FAIL 失败
 */
esp_err_t Timer_Deinit(void)
{
    esp_err_t ret = ESP_OK;

    // 停止定时器
    if (timer_handle != NULL) {
        if (is_running) {
            ret = esp_timer_stop(timer_handle);
            if (ret != ESP_OK) {
                ESP_LOGE(TAG, "定时器停止失败: %s", esp_err_to_name(ret));
            }
        }

        ret = esp_timer_delete(timer_handle);
        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "定时器删除失败: %s", esp_err_to_name(ret));
        }
        timer_handle = NULL;
    }

    // 删除事件处理任务
    if (event_task_handle != NULL) {
        vTaskDelete(event_task_handle);
        event_task_handle = NULL;
    }

    // 删除事件队列
    if (event_queue != NULL) {
        vQueueDelete(event_queue);
        event_queue = NULL;
    }

    is_running = false;
    timer_callback = NULL;

    ESP_LOGI(TAG, "定时器反初始化完成");
    return ret;
}
