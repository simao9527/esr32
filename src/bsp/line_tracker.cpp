#include "line_tracker.h"

// 初始化状态
static bool s_initialized = false;

// GPIO数组
static const uint8_t s_tracker_gpios[LINE_TRACKER_NUM] =
{
    LINE_TRACKER_GPIO_1,
    LINE_TRACKER_GPIO_2,
    LINE_TRACKER_GPIO_3,
    LINE_TRACKER_GPIO_4,
    LINE_TRACKER_GPIO_5
};


// 初始化
bool line_tracker_init(void)
{
    if (s_initialized)
    {
        Serial.println("Line tracker already initialized");
        return true;
    }

    for (int i = 0; i < LINE_TRACKER_NUM; i++)
    {
        pinMode(s_tracker_gpios[i], INPUT_PULLUP);
    }

    s_initialized = true;

    Serial.println("Line tracker initialized successfully");

    return true;
}


// 读取单个传感器
bool line_tracker_read_sensor(uint8_t sensor_num)
{
    if (!s_initialized)
    {
        Serial.println("Line tracker not initialized");
        return false;
    }

    if (sensor_num < 1 || sensor_num > LINE_TRACKER_NUM)
    {
        Serial.println("Invalid sensor number");
        return false;
    }

    int level = digitalRead(s_tracker_gpios[sensor_num - 1]);

    // 低电平 = 检测到黑线
    return (level == LOW);
}


// 读取全部传感器
bool line_tracker_read_all(line_tracker_state_t *state)
{
    if (!s_initialized)
    {
        Serial.println("Line tracker not initialized");
        return false;
    }

    if (state == NULL)
    {
        return false;
    }

    state->sensor1 = line_tracker_read_sensor(1);
    state->sensor2 = line_tracker_read_sensor(2);
    state->sensor3 = line_tracker_read_sensor(3);
    state->sensor4 = line_tracker_read_sensor(4);
    state->sensor5 = line_tracker_read_sensor(5);

    return true;
}


// 获取原始数组
bool line_tracker_get_raw_values(bool values[LINE_TRACKER_NUM])
{
    if (!s_initialized)
    {
        Serial.println("Line tracker not initialized");
        return false;
    }

    if (values == NULL)
    {
        return false;
    }

    for (int i = 0; i < LINE_TRACKER_NUM; i++)
    {
        int level = digitalRead(s_tracker_gpios[i]);

        values[i] = (level == LOW);
    }

    return true;
}


// 检测是否有线
bool line_tracker_detect_line(void)
{
    if (!s_initialized)
    {
        return false;
    }

    for (int i = 0; i < LINE_TRACKER_NUM; i++)
    {
        if (line_tracker_read_sensor(i + 1))
        {
            return true;
        }
    }

    return false;
}


// 反初始化
void line_tracker_deinit(void)
{
    if (!s_initialized)
    {
        return;
    }

    s_initialized = false;

    Serial.println("Line tracker deinitialized");
}