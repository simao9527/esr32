/**
 * @file xunji.c
 * @brief 五路寻迹传感器驱动实现文件
 * @details 提供基于五路红外传感器的寻迹功能，支持读取传感器状态和判断轨迹位置
 */

#include "xunji.h"
#include "driver/gpio.h"
#include "esp_log.h"

static const char *TAG = "XUNJI";

// 默认寻迹传感器配置
static XunjiConfig xunji_config = {
    .pins = {
        XUNJI_SENSOR1_PIN,
        XUNJI_SENSOR2_PIN,
        XUNJI_SENSOR3_PIN,
        XUNJI_SENSOR4_PIN,
        XUNJI_SENSOR5_PIN
    },
    .active_level = XUNJI_BLACK_LINE  // 默认黑线（高电平有效）
};

/**
 * @brief 寻迹传感器初始化函数
 * @details 初始化寻迹传感器的GPIO引脚为输入模式
 */
void Xunji_Init(void)
{
    // 配置所有传感器引脚为输入模式
    for (int i = 0; i < XUNJI_SENSOR_COUNT; i++) {
        gpio_config_t io_conf = {
            .pin_bit_mask = (1ULL << xunji_config.pins[i]),
            .mode = GPIO_MODE_INPUT,
            .pull_up_en = GPIO_PULLUP_DISABLE,
            .pull_down_en = GPIO_PULLDOWN_ENABLE,
            .intr_type = GPIO_INTR_DISABLE
        };
        gpio_config(&io_conf);
    }

    ESP_LOGI(TAG, "Xunji sensors initialized");
}

/**
 * @brief 读取寻迹传感器状态
 * @details 读取所有传感器的当前状态
 *
 * @return 寻迹传感器状态结构体
 */
XunjiSensorState Xunji_ReadSensors(void)
{
    XunjiSensorState state;

    // 读取所有传感器的状态
    state.sensor1 = (gpio_get_level(xunji_config.pins[0]) == xunji_config.active_level);
    state.sensor2 = (gpio_get_level(xunji_config.pins[1]) == xunji_config.active_level);
    state.sensor3 = (gpio_get_level(xunji_config.pins[2]) == xunji_config.active_level);
    state.sensor4 = (gpio_get_level(xunji_config.pins[3]) == xunji_config.active_level);
    state.sensor5 = (gpio_get_level(xunji_config.pins[4]) == xunji_config.active_level);

    return state;
}

/**
 * @brief 获取轨迹位置
 * @details 根据传感器状态判断轨迹相对于车辆的位置
 *
 * @return 轨迹位置枚举值
 */
XunjiPosition Xunji_GetPosition(void)
{
    XunjiSensorState state = Xunji_ReadSensors();

    // 优先判断中间传感器
    if (state.sensor3) {
        // 中间传感器检测到线，检查左右传感器判断偏移
        if (state.sensor2) {
            return XUNJI_POS_LEFT;
        } else if (state.sensor4) {
            return XUNJI_POS_RIGHT;
        } else {
            return XUNJI_POS_CENTER;
        }
    }

    // 检查左侧传感器
    if (state.sensor2) {
        if (state.sensor1) {
            return XUNJI_POS_FAR_LEFT;
        }
        return XUNJI_POS_LEFT;
    }

    // 检查右侧传感器
    if (state.sensor4) {
        if (state.sensor5) {
            return XUNJI_POS_FAR_RIGHT;
        }
        return XUNJI_POS_RIGHT;
    }

    // 检查最左侧和最右侧传感器
    if (state.sensor1) {
        return XUNJI_POS_FAR_LEFT;
    }
    if (state.sensor5) {
        return XUNJI_POS_FAR_RIGHT;
    }

    // 所有传感器都没有检测到线
    return XUNJI_POS_LOST;
}

/**
 * @brief 判断是否丢失轨迹
 * @details 判断是否所有传感器都没有检测到轨迹
 *
 * @return true表示丢失轨迹，false表示检测到轨迹
 */
bool Xunji_IsLineLost(void)
{
    XunjiSensorState state = Xunji_ReadSensors();

    // 如果所有传感器都没有检测到线，则认为丢失轨迹
    return !(state.sensor1 || state.sensor2 || state.sensor3 || 
             state.sensor4 || state.sensor5);
}

/**
 * @brief 获取轨迹偏移量
 * @details 计算轨迹相对于中心的偏移量，用于PID控制
 *         使用加权平均算法计算更精确的偏移量
 *
 * @return 偏移量，负值表示偏左，正值表示偏右，0表示居中
 *         范围：-100到100
 */
int8_t Xunji_GetOffset(void)
{
    XunjiSensorState state = Xunji_ReadSensors();

    // 计算加权偏移量，使用更精细的权重
    // 权重：sensor1=-50, sensor2=-25, sensor3=0, sensor4=25, sensor5=50
    int16_t weighted_sum = 0;
    int16_t total_weight = 0;

    if (state.sensor1) {
        weighted_sum -= 50;
        total_weight += 1;
    }
    if (state.sensor2) {
        weighted_sum -= 25;
        total_weight += 1;
    }
    if (state.sensor3) {
        weighted_sum += 0;
        total_weight += 1;
    }
    if (state.sensor4) {
        weighted_sum += 25;
        total_weight += 1;
    }
    if (state.sensor5) {
        weighted_sum += 50;
        total_weight += 1;
    }

    // 如果没有检测到线，返回0（保持上次状态或停止）
    if (total_weight == 0) {
        return 0;
    }

    // 计算平均偏移量
    int8_t offset = (int8_t)(weighted_sum / total_weight);

    return offset;
}

/**
 * @brief 获取单个传感器状态
 * @details 获取指定传感器的状态
 *
 * @param index 传感器索引（0-4）
 * @return 传感器状态，true表示检测到线，false表示未检测到线
 */
bool Xunji_GetSensor(uint8_t index)
{
    if (index >= XUNJI_SENSOR_COUNT) {
        ESP_LOGW(TAG, "Invalid sensor index: %d", index);
        return false;
    }

    return (gpio_get_level(xunji_config.pins[index]) == xunji_config.active_level);
}
