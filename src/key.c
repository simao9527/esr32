
#include "key.h"
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"

// 按键消抖时间(ms)
#define KEY_DEBOUNCE_TIME 50

// 长按时间阈值(ms)
#define KEY_LONG_PRESS_TIME 1000

// 按键状态变量
static SpeedLevel currentSpeedLevel = SPEED_LOW; // 当前档位
static KeyState lastKeyState = KEY_RELEASED;     // 上一次按键状态
static uint32_t lastPressTime = 0;              // 上次按下时间

// 日志标签
static const char *TAG = "KEY";

/**
 * @brief 按键初始化函数
 * @param 无
 * @return 无
 * @note 初始化GPIO 35为输入模式，启用上拉电阻
 */
void Key_Init(void)
{
    // 配置GPIO 35为输入模式，启用上拉电阻
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << KEY_PIN),      // 选择GPIO 35
        .mode = GPIO_MODE_INPUT,                // 设置为输入模式
        .pull_up_en = GPIO_PULLUP_ENABLE,       // 启用上拉电阻
        .pull_down_en = GPIO_PULLDOWN_DISABLE,  // 禁用下拉电阻
        .intr_type = GPIO_INTR_DISABLE          // 禁用中断
    };

    // 应用GPIO配置
    gpio_config(&io_conf);

    ESP_LOGI(TAG, "Key initialized on GPIO %d", KEY_PIN);
}

/**
 * @brief 获取按键状态
 * @param 无
 * @return KeyState 当前按键状态
 * @note 读取GPIO状态并返回按键状态
 */
KeyState Key_GetState(void)
{
    // 读取GPIO状态，低电平表示按下
    return gpio_get_level(KEY_PIN) == 0 ? KEY_PRESSED : KEY_RELEASED;
}

/**
 * @brief 获取当前档位
 * @param 无
 * @return SpeedLevel 当前速度档位
 */
SpeedLevel Key_GetSpeedLevel(void)
{
    return currentSpeedLevel;
}

/**
 * @brief 设置当前档位
 * @param level 要设置的档位
 * @return 无
 */
void Key_SetSpeedLevel(SpeedLevel level)
{
    // 确保档位在有效范围内
    if (level >= SPEED_LOW && level <= SPEED_HIGH)
    {
        currentSpeedLevel = level;
        ESP_LOGI(TAG, "Speed level set to %d", level);
    }
}

/**
 * @brief 按键扫描函数，在主循环中调用
 * @param 无
 * @return 无
 * @note 检测按键按下事件，实现档位切换功能
 */
void Key_Scan(void)
{
    KeyState currentState = Key_GetState();

    // 检测按键按下事件（从释放到按下）
    if (currentState == KEY_PRESSED && lastKeyState == KEY_RELEASED)
    {
        // 记录按下时间
        lastPressTime = xTaskGetTickCount() * portTICK_PERIOD_MS;
    }
    // 检测按键释放事件（从按下到释放）
    else if (currentState == KEY_RELEASED && lastKeyState == KEY_PRESSED)
    {
        // 计算按键按下的持续时间
        uint32_t pressDuration = (xTaskGetTickCount() * portTICK_PERIOD_MS) - lastPressTime;

        // 如果按下时间超过消抖时间，认为是有效按键
        if (pressDuration > KEY_DEBOUNCE_TIME)
        {
            // 升档位，如果已经是最高档则回到最低档
            currentSpeedLevel = (currentSpeedLevel + 1) % 3;

            ESP_LOGI(TAG, "Speed level changed to %d", currentSpeedLevel);
        }
    }

    // 更新上一次按键状态
    lastKeyState = currentState;
}
