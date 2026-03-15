#include "car_track.h"
#include "tb6612_driver.h"
#include "line_tracker.h"
#include "esp_log.h"

#include <stdlib.h>
#include <string.h>

static const char *TAG = "CAR_TRACK";

/* 默认参数 */

#define DEFAULT_TRACK_SPEED 120
#define DEFAULT_TRACK_MAX_SPEED 170
#define DEFAULT_TRACK_MIN_SPEED 60

#define DEFAULT_MAX_LOST_COUNT 15
#define DEFAULT_CHECK_INTERVAL_MS 20

/* PID默认参数 */
#define DEFAULT_KP 22.0f
#define DEFAULT_KI 0.0f
#define DEFAULT_KD 10.0f

/* PID积分限幅 */
#define PID_INTEGRAL_LIMIT 100


/* =============================
   速度档位参数
   ============================= */

#define LOW_SPEED_BASE 90
#define LOW_SPEED_MAX  120
#define LOW_SPEED_MIN  60

#define MID_SPEED_BASE 120
#define MID_SPEED_MAX  170
#define MID_SPEED_MIN  70

#define HIGH_SPEED_BASE 150
#define HIGH_SPEED_MAX  210
#define HIGH_SPEED_MIN  80


static uint8_t g_speed_level = CAR_TRACK_SPEED_MID;
static uint8_t g_track_speed = MID_SPEED_BASE;
static uint8_t g_track_max_speed = MID_SPEED_MAX;
static uint8_t g_track_min_speed = MID_SPEED_MIN;


/* =============================
   速度档位函数
   ============================= */

uint8_t car_track_set_speed_level(uint8_t level)
{

    if (level == CAR_TRACK_SPEED_LOW)
    {

        g_speed_level = CAR_TRACK_SPEED_LOW;

        g_track_speed = LOW_SPEED_BASE;
        g_track_max_speed = LOW_SPEED_MAX;
        g_track_min_speed = LOW_SPEED_MIN;

    }
    else if (level == CAR_TRACK_SPEED_MID)
    {

        g_speed_level = CAR_TRACK_SPEED_MID;

        g_track_speed = MID_SPEED_BASE;
        g_track_max_speed = MID_SPEED_MAX;
        g_track_min_speed = MID_SPEED_MIN;

    }
    else if (level == CAR_TRACK_SPEED_HIGH)
    {

        g_speed_level = CAR_TRACK_SPEED_HIGH;

        g_track_speed = HIGH_SPEED_BASE;
        g_track_max_speed = HIGH_SPEED_MAX;
        g_track_min_speed = HIGH_SPEED_MIN;

    }

    ESP_LOGI(TAG,"Speed level set: %d",g_speed_level);

    return g_speed_level;
}



static void car_track_task(void *pvParameters)
{

    car_track_config_t *config = (car_track_config_t *)pvParameters;

    uint8_t track_speed =
        config ? config->track_speed : g_track_speed;

    uint8_t track_max_speed =
        config ? config->track_max_speed : g_track_max_speed;

    uint8_t track_min_speed =
        config ? config->track_min_speed : g_track_min_speed;

    uint32_t max_lost_count =
        config ? config->max_lost_count : DEFAULT_MAX_LOST_COUNT;

    uint32_t check_interval =
        config ? config->check_interval_ms : DEFAULT_CHECK_INTERVAL_MS;

    float kp = config ? config->kp : DEFAULT_KP;
    float ki = config ? config->ki : DEFAULT_KI;
    float kd = config ? config->kd : DEFAULT_KD;

    /* PID变量 */

    float error = 0;
    float last_error = 0;
    float integral = 0;
    float derivative = 0;
    float output = 0;

    int8_t last_deviation = 0;

    uint32_t lost_line_count = 0;

    int16_t last_left_speed = track_speed;
    int16_t last_right_speed = track_speed;

    ESP_LOGI(TAG, "Track mode task started");

    while (1)
    {

        line_tracker_state_t tracker_state;

        if (line_tracker_read_all(&tracker_state) == ESP_OK)
        {

            bool s1 = tracker_state.sensor1;
            bool s2 = tracker_state.sensor2;
            bool s3 = tracker_state.sensor3;
            bool s4 = tracker_state.sensor4;
            bool s5 = tracker_state.sensor5;

            /* =====================
               计算偏差
            ===================== */

            int8_t deviation = 0;
            bool line_detected = false;

            int sensor_count =
                (s1 ? 1 : 0) +
                (s2 ? 1 : 0) +
                (s3 ? 1 : 0) +
                (s4 ? 1 : 0) +
                (s5 ? 1 : 0);

            if (sensor_count == 0)
            {
                line_detected = false;
            }
            else if (sensor_count == 1)
            {
                if (s1) deviation = -2;
                else if (s2) deviation = -1;
                else if (s3) deviation = 0;
                else if (s4) deviation = 1;
                else if (s5) deviation = 2;

                line_detected = true;
            }
            else
            {

                float weighted_pos = 0;
                int weight_sum = 0;

                if (s1){ weighted_pos += -2.0f * 3; weight_sum += 3; }
                if (s2){ weighted_pos += -0.75f * 2; weight_sum += 2; }
                if (s3){ weighted_pos += 0.0f * 4; weight_sum += 4; }
                if (s4){ weighted_pos += 0.75f * 2; weight_sum += 2; }
                if (s5){ weighted_pos += 2.0f * 3; weight_sum += 3; }

                if (weight_sum > 0)
                {
                    float avg_pos = weighted_pos / weight_sum;

                    if (avg_pos < -1.0f) deviation = -2;
                    else if (avg_pos < -0.4f) deviation = -1;
                    else if (avg_pos < 0.4f) deviation = 0;
                    else if (avg_pos < 1.0f) deviation = 1;
                    else deviation = 2;

                    line_detected = true;
                }
            }

            /* =====================
               PID控制
            ===================== */

            if (line_detected)
            {

                lost_line_count = 0;

                error = deviation;

                integral += error;

                if (integral > PID_INTEGRAL_LIMIT)
                    integral = PID_INTEGRAL_LIMIT;
                if (integral < -PID_INTEGRAL_LIMIT)
                    integral = -PID_INTEGRAL_LIMIT;

                derivative = error - last_error;

                output =
                    kp * error +
                    ki * integral +
                    kd * derivative;

                last_error = error;

                int16_t left_speed =
                    track_speed - output;

                int16_t right_speed =
                    track_speed + output;

                if (left_speed > track_max_speed)
                    left_speed = track_max_speed;

                if (right_speed > track_max_speed)
                    right_speed = track_max_speed;

                if (left_speed < track_min_speed)
                    left_speed = track_min_speed;

                if (right_speed < track_min_speed)
                    right_speed = track_min_speed;

                last_left_speed = left_speed;
                last_right_speed = right_speed;

                last_deviation = deviation;

                tb6612_set_motor(left_speed, right_speed);

                ESP_LOGD(TAG,
                         "dev=%d out=%.2f L=%d R=%d",
                         deviation,
                         output,
                         left_speed,
                         right_speed);
            }

            /* =====================
               丢线处理
            ===================== */

            else
            {

                lost_line_count++;

                if (lost_line_count < max_lost_count)
                {
                    tb6612_set_motor(
                        last_left_speed,
                        last_right_speed);
                }
                else
                {
                    tb6612_car_stop();
                    ESP_LOGW(TAG,
                             "Lost line");
                }
            }
        }
        else
        {
            tb6612_car_stop();
            ESP_LOGW(TAG,
                     "Sensor read failed");
        }

        vTaskDelay(check_interval / portTICK_PERIOD_MS);
    }

    tb6612_car_stop();

    ESP_LOGI(TAG, "Track task stopped");

    if (config)
        free(config);

    vTaskDelete(NULL);
}



/* 启动任务 */

TaskHandle_t car_track_start(const car_track_config_t *config)
{

    TaskHandle_t task_handle = NULL;

    car_track_config_t *config_copy = NULL;

    if (config)
    {

        config_copy =
            (car_track_config_t *)malloc(sizeof(car_track_config_t));

        if (!config_copy)
        {
            ESP_LOGE(TAG, "malloc failed");
            return NULL;
        }

        *config_copy = *config;
    }

    xTaskCreate(
        car_track_task,
        "car_track",
        4096,
        config_copy,
        5,
        &task_handle);

    return task_handle;
}



/* 停止任务 */

void car_track_stop(TaskHandle_t task_handle)
{
    if (task_handle != NULL)
    {
        /* 模式切换后任务会自动退出 */
    }
}